#include "renderer.hpp"

void Renderer::fallback_fix()
{
    fallback_image.resize(2);
    fallback_image_view.resize(2);

    VkExtent2D lfallback_extent = {1, 1};

    create_image(fallback_image[0], VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, lfallback_extent);
    fallback_image_view[0] = create_depth_image_view(fallback_image[0], VK_FORMAT_D32_SFLOAT);

    create_cubemap(fallback_image[1], lfallback_extent);
    fallback_image_view[1] = create_cubemap_views(fallback_image[1]);

    //transitioning the fallback image to SHADER_READ_ONLY_OPTIMAL immediately
    VkCommandBuffer temp_cmd;
    begin_temp_command_buffer(temp_cmd);

    transition_image(
        temp_cmd,
        fallback_image[0],
        VK_IMAGE_ASPECT_DEPTH_BIT,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
        VK_ACCESS_2_NONE,
        VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
        VK_ACCESS_2_SHADER_READ_BIT
    );

    transition_image(
        temp_cmd,
        fallback_image[1],
        VK_IMAGE_ASPECT_DEPTH_BIT,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
        VK_ACCESS_2_NONE,
        VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
        VK_ACCESS_2_SHADER_READ_BIT
    );

    end_temp_command_buffer(temp_cmd);
}


void Renderer::create_main_pass_descriptors()
{
    std::cout << "main pass layout" << std::endl;
    const SetLayoutInfo& set_info = set_layout_infos.at(0);

    allocate_descriptor_set(set_info.layout, main_pass_descriptor_sets, static_cast<uint32_t>(swap_chain_images.size()));

    allocate_shadow_map_images_and_views();
    allocate_cubemap_images_and_views();

    fallback_fix();

    create_and_map_descriptor_buffer(sizeof(LightBufferObject), swap_chain_images.size(), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, dir_lights_buffer);
    create_and_map_descriptor_buffer(sizeof(CameraUBO), swap_chain_images.size(), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, camera_ubo_buffer);
    create_and_map_descriptor_buffer(sizeof(PointLightBufferObject), swap_chain_images.size(), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, point_lights_buffer);

    for (uint32_t i = 0; i < swap_chain_images.size(); i++)
    {
        update_dir_lights_ubo(i);
        update_camera_ubo(i);
        update_point_lights_buffer(i);
    }

    for (int frame = 0; frame < swap_chain_images.size(); frame++)
    {
        std::vector<VkDescriptorImageInfo> frame_image_infos;
        std::vector<VkDescriptorImageInfo> point_light_frame_image_infos;
        
        for (int i = 0; i < MAX_DIRECTIONAL_LIGHTS; i++)
        {
            if (i < directional_lights.size())
            {
                assert(directional_lights[i].depth_image_views[frame] != VK_NULL_HANDLE && "active light has a null image view");

                frame_image_infos.push_back(make_image_info(directional_lights[i].depth_image_views[frame], shadow_sampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
            }
            else 
            {
               frame_image_infos.push_back(make_image_info(fallback_image_view[0], shadow_sampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)); 
            }
        }

        for (int i = 0; i < MAX_POINT_LIGHTS; i++)
        {
            if (i < point_lights.size())
            {
                assert(point_lights[i].depth_image_views[frame] != VK_NULL_HANDLE && "active point light has a null image view");
                point_light_frame_image_infos.push_back(make_image_info(point_lights[i].depth_image_views[frame], shadow_sampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
            }
            else 
            {
                point_light_frame_image_infos.push_back(make_image_info(fallback_image_view[1], shadow_sampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)); 
            }
        }

        BindingData sampler_bd{};
        sampler_bd.name = "shadow_maps";
        sampler_bd.buffers = {};
        sampler_bd.images = frame_image_infos;

        BindingData point_sampler_bd{};
        point_sampler_bd.name = "point_light_shadow_maps";
        point_sampler_bd.buffers = {};
        point_sampler_bd.images = point_light_frame_image_infos;

        BindingData light_buffer_object_bd{};
        light_buffer_object_bd.name = "lbo";
        light_buffer_object_bd.images = {};
        light_buffer_object_bd.buffers = {make_buffer_info(dir_lights_buffer, frame)};

        BindingData point_light_buffer_object_bd{};
        point_light_buffer_object_bd.name = "pbo";
        point_light_buffer_object_bd.images = {};
        point_light_buffer_object_bd.buffers = {make_buffer_info(point_lights_buffer, frame)};

        BindingData camera_ubo_bd{};
        camera_ubo_bd.name = "cbo";
        camera_ubo_bd.images = {};
        camera_ubo_bd.buffers = {make_buffer_info(camera_ubo_buffer, frame)};

        std::vector<BindingData> binding_data = {sampler_bd, point_sampler_bd, light_buffer_object_bd, point_light_buffer_object_bd, camera_ubo_bd};

        write_set(main_pass_descriptor_sets[frame], set_info, binding_data);
    }
}

void Renderer::update_dir_lights_ubo(uint32_t current_frame)
{
    LightBufferObject ubo{};
    ubo.active_lights = static_cast<int>(directional_lights.size());
    for (size_t i = 0; i < directional_lights.size(); ++i)
    {
        ubo.dir_lights[i] = directional_lights[i].to_gpu();
    }

    VkDeviceSize offset = current_frame * dir_lights_buffer.padded_size;
    std::byte* dst = static_cast<std::byte*>(dir_lights_buffer.mapped) + offset;

    assert(dir_lights_buffer.mapped != nullptr && "VMA memory was never mapped");

    memcpy(dst, &ubo, sizeof(LightBufferObject));
    vmaFlushAllocation(allocator, dir_lights_buffer.allocation, offset, dir_lights_buffer.padded_size);
}

void Renderer::update_camera_ubo(uint32_t current_frame)
{
    CameraUBO ubo{};
    ubo.camera_pos = camera.position;
    ubo.viewproj = perspective_projection_matrix * camera.get_view_matrix();

    VkDeviceSize offset = current_frame * camera_ubo_buffer.padded_size;
    std::byte* dst = static_cast<std::byte*>(camera_ubo_buffer.mapped) + offset;

    memcpy(dst, &ubo, sizeof(CameraUBO));
    vmaFlushAllocation(allocator, camera_ubo_buffer.allocation, offset, camera_ubo_buffer.padded_size);
}

void Renderer::update_point_lights_buffer(uint32_t current_frame)
{
    PointLightBufferObject ubo{};
    ubo.active_lights = static_cast<int>(point_lights.size());
    for (int i = 0; i < point_lights.size(); i++)
    {
        ubo.point_lights[i] = point_lights[i].to_gpu();
    }

    VkDeviceSize offset = current_frame * point_lights_buffer.padded_size;
    std::byte* dst = static_cast<std::byte*>(point_lights_buffer.mapped) + offset;

    assert(point_lights_buffer.mapped != nullptr && "VMA memory was never mapped for point lights buffer");

    memcpy(dst, &ubo, sizeof(PointLightBufferObject));
    vmaFlushAllocation(allocator, point_lights_buffer.allocation, offset, point_lights_buffer.padded_size);
}

void Renderer::main_pass(uint32_t current_frame, uint32_t image_index)
{
    VkRenderingAttachmentInfo color_attachment{};
    
    color_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;

    color_attachment.clearValue.color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    color_attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    color_attachment.imageView = swap_chain_image_views[image_index];


    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    VkRenderingAttachmentInfo depth_attachment{};

    depth_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;

    depth_attachment.clearValue.depthStencil = {1.0f, 0};
    depth_attachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depth_attachment.imageView = depth_image_views[current_frame];

    depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    VkRenderingInfo main_pass_rendering_info{};
    main_pass_rendering_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;

    main_pass_rendering_info.layerCount = 1;

    main_pass_rendering_info.colorAttachmentCount = 1;
    main_pass_rendering_info.pColorAttachments = &color_attachment;

    main_pass_rendering_info.pDepthAttachment = &depth_attachment;
    main_pass_rendering_info.pStencilAttachment = nullptr;
    main_pass_rendering_info.renderArea.extent = swap_chain_extent;
    main_pass_rendering_info.renderArea.offset = {0, 0};

    vkCmdBeginRendering(main_command_buffers[current_frame], &main_pass_rendering_info);

    vkCmdBindPipeline(main_command_buffers[current_frame], VK_PIPELINE_BIND_POINT_GRAPHICS, main_pipeline);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swap_chain_extent.width);
    viewport.height = static_cast<float>(swap_chain_extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(main_command_buffers[current_frame], 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swap_chain_extent;
    vkCmdSetScissor(main_command_buffers[current_frame], 0, 1, &scissor);

    vkCmdBindDescriptorSets(main_command_buffers[current_frame], VK_PIPELINE_BIND_POINT_GRAPHICS, main_pipeline_layout, 0, 1, &main_pass_descriptor_sets[current_frame], 0, nullptr);

    for (const auto& object: render_objects)
    {
        VkDeviceSize offsets = 0;

        if (object.has_texture)
        {
            vkCmdBindDescriptorSets(main_command_buffers[current_frame], VK_PIPELINE_BIND_POINT_GRAPHICS, main_pipeline_layout, 1, 1, &object.material_descriptor_set, 0, nullptr);
        }

        vkCmdBindVertexBuffers(main_command_buffers[current_frame], 0, 1, &object.mesh->vertex_buffer.buffer, &offsets);
        vkCmdBindIndexBuffer(main_command_buffers[current_frame], object.mesh->index_buffer.buffer, 0, VK_INDEX_TYPE_UINT16);

        Mat4IntPc pc{};

        pc.has_texture = object.has_texture;
        pc.model = object.model;

        vkCmdPushConstants(main_command_buffers[current_frame], main_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(Mat4IntPc), &pc);

        vkCmdDrawIndexed(main_command_buffers[current_frame], object.mesh->index_count, 1, 0, 0, 0);  
    }

    vkCmdEndRendering(main_command_buffers[current_frame]);

}

void Renderer::create_main_pipeline()
{
    std::vector<char> vert_code = readFile(main_vertex_shader_filepath);
    std::vector<char> frag_code = readFile(main_fragment_shader_filepath);

    VkShaderModule vert_module = createShaderModule(vert_code, device);
    VkShaderModule frag_module = createShaderModule(frag_code, device);

    VkFormat color_format = VK_FORMAT_B8G8R8A8_SRGB; 
    VkFormat depth_format = VK_FORMAT_D32_SFLOAT;    

    VkPipelineRenderingCreateInfo pipeline_rendering_create_info{};
    pipeline_rendering_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    pipeline_rendering_create_info.colorAttachmentCount = 1; 
    pipeline_rendering_create_info.pColorAttachmentFormats = &color_format;
    pipeline_rendering_create_info.depthAttachmentFormat = depth_format;

    VkPipelineShaderStageCreateInfo vertex_shader_stage_create_info{};
    vertex_shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertex_shader_stage_create_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertex_shader_stage_create_info.module = vert_module;
    vertex_shader_stage_create_info.pName = "main";

    VkPipelineShaderStageCreateInfo fragment_shader_stage_create_info{};
    fragment_shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragment_shader_stage_create_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragment_shader_stage_create_info.module = frag_module;
    fragment_shader_stage_create_info.pName = "main";

    VkPipelineShaderStageCreateInfo shader_stage_create_infos[] = {vertex_shader_stage_create_info, fragment_shader_stage_create_info};

    VkVertexInputBindingDescription vertex_input_binding_description{};
    vertex_input_binding_description.binding = 0;
    vertex_input_binding_description.stride = static_cast<uint32_t>(sizeof(Vertex));    
    vertex_input_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::vector<VkVertexInputBindingDescription> vertex_input_binding_descriptions = {vertex_input_binding_description};

    VkVertexInputAttributeDescription position_input_attribute_description{};
    position_input_attribute_description.location = 0;
    position_input_attribute_description.binding = 0;
    position_input_attribute_description.offset = static_cast<uint32_t>(offsetof(Vertex, position));
    position_input_attribute_description.format = VK_FORMAT_R32G32B32_SFLOAT;
    
    VkVertexInputAttributeDescription color_input_attribute_description{};
    color_input_attribute_description.location = 1;
    color_input_attribute_description.binding = 0;
    color_input_attribute_description.offset = static_cast<uint32_t>(offsetof(Vertex, color));
    color_input_attribute_description.format = VK_FORMAT_R32G32B32_SFLOAT;

    VkVertexInputAttributeDescription normal_input_attribute_description{};
    normal_input_attribute_description.location = 2;
    normal_input_attribute_description.binding = 0;
    normal_input_attribute_description.offset = static_cast<uint32_t>(offsetof(Vertex, normal));
    normal_input_attribute_description.format = VK_FORMAT_R32G32B32_SFLOAT;

    VkVertexInputAttributeDescription uv_input_attribute_description{};
    uv_input_attribute_description.location = 3;
    uv_input_attribute_description.binding = 0;
    uv_input_attribute_description.offset = static_cast<uint32_t>(offsetof(Vertex, uv));
    uv_input_attribute_description.format = VK_FORMAT_R32G32_SFLOAT;

    std::vector<VkVertexInputAttributeDescription> vertex_input_attribute_descriptions = {position_input_attribute_description, color_input_attribute_description, normal_input_attribute_description, uv_input_attribute_description};

    VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info{};
    vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    vertex_input_state_create_info.vertexBindingDescriptionCount = 1;
    vertex_input_state_create_info.pVertexBindingDescriptions = vertex_input_binding_descriptions.data();

    vertex_input_state_create_info.vertexAttributeDescriptionCount = 4;
    vertex_input_state_create_info.pVertexAttributeDescriptions = vertex_input_attribute_descriptions.data();

    VkPipelineInputAssemblyStateCreateInfo input_assembly{};
    input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewport_state{};
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState color_blend_attachment{};
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = VK_FALSE;    

    VkPipelineColorBlendStateCreateInfo color_blending_create_info{};
    color_blending_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blending_create_info.logicOpEnable = VK_FALSE;
    color_blending_create_info.logicOp = VK_LOGIC_OP_COPY;
    color_blending_create_info.attachmentCount = 1;
    color_blending_create_info.pAttachments = &color_blend_attachment;
    color_blending_create_info.blendConstants[0] = 0.0f;
    color_blending_create_info.blendConstants[1] = 0.0f;
    color_blending_create_info.blendConstants[2] = 0.0f;
    color_blending_create_info.blendConstants[3] = 0.0f;

    VkPipelineDepthStencilStateCreateInfo depth_stencil{};
    depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil.depthTestEnable = VK_TRUE;           
    depth_stencil.depthWriteEnable = VK_TRUE;          
    depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS; 
    depth_stencil.depthBoundsTestEnable = VK_FALSE;
    depth_stencil.stencilTestEnable = VK_FALSE;

    std::vector<VkDynamicState> dynamic_states = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamic_state{};
    dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
    dynamic_state.pDynamicStates = dynamic_states.data();


    VkPushConstantRange push_constant1{};
    push_constant1.offset = 0;
    push_constant1.size = sizeof(Mat4IntPc);
    push_constant1.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;


    std::vector<VkPushConstantRange> push_constant_ranges = {push_constant1};

    std::cout << "assigning set layouts to main pipeline" << std::endl;
    VkDescriptorSetLayout set_layouts[] = {set_layout_infos.at(0).layout, set_layout_infos.at(1).layout};

    VkPipelineLayoutCreateInfo pipeline_layout_info{};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 2;
    pipeline_layout_info.pSetLayouts = set_layouts;
    pipeline_layout_info.pushConstantRangeCount = static_cast<uint32_t>(push_constant_ranges.size());
    pipeline_layout_info.pPushConstantRanges = push_constant_ranges.data();

    if (vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr, &main_pipeline_layout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create pipeline layout");
    }

    resources.push_back(ResourceTypes::PipelineLayouts, main_pipeline_layout);

    VkGraphicsPipelineCreateInfo pipeline_create_info{};
    pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_create_info.pNext = &pipeline_rendering_create_info;
    pipeline_create_info.stageCount = 2;
    pipeline_create_info.pStages = shader_stage_create_infos;
    pipeline_create_info.pVertexInputState = &vertex_input_state_create_info;
    pipeline_create_info.pInputAssemblyState = &input_assembly;
    pipeline_create_info.pViewportState = &viewport_state;
    pipeline_create_info.pRasterizationState = &rasterizer;
    pipeline_create_info.pMultisampleState = &multisampling;
    pipeline_create_info.pColorBlendState = &color_blending_create_info;
    pipeline_create_info.pDepthStencilState = &depth_stencil;
    pipeline_create_info.pDynamicState = &dynamic_state;
    pipeline_create_info.layout = main_pipeline_layout;
    pipeline_create_info.renderPass = VK_NULL_HANDLE;
    pipeline_create_info.subpass = 0;
    pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &main_pipeline) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create graphics pipeline");
    }

    resources.push_back(ResourceTypes::Pipelines, main_pipeline);

    vkDestroyShaderModule(device, vert_module, nullptr);
    vkDestroyShaderModule(device, frag_module, nullptr);
}