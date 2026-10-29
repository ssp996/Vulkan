#include "renderer.hpp"

void Renderer::create_directional_light_descriptors()
{
    const SetLayoutInfo& set_info = set_layout_infos.at(0);

    allocate_descriptor_set(set_info.layout, shadow_map_descriptor_sets, static_cast<uint32_t>(swap_chain_images.size()));

    allocate_shadow_map_images_and_views();

    for (int frame = 0; frame < swap_chain_images.size(); frame++)
    {
        std::vector<VkDescriptorImageInfo> frame_image_infos;
        for (const auto& light: directional_lights)
        {
            frame_image_infos.push_back(make_image_info(light.depth_image_views[frame], shadow_sampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
        }

        BindingData sampler_bd{};
        sampler_bd.name = "dirShadowMaps";
        sampler_bd.buffers = {};
        sampler_bd.images = frame_image_infos;

        BindingData light_buffer_object_bd{};
        light_buffer_object_bd.name = "dirLights";
        light_buffer_object_bd.images = {};
        light_buffer_object_bd.buffers = {make_buffer_info(dir_lights_buffer, frame)};

        std::vector<BindingData> binding_data = {sampler_bd, light_buffer_object_bd};
        
        write_set(shadow_map_descriptor_sets[frame], set_info, binding_data);
    }
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

    VkRenderingAttachmentInfo stencil_attachment{};
    stencil_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    stencil_attachment.imageView = depth_image_views[current_frame];          
    stencil_attachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    stencil_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    stencil_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    stencil_attachment.clearValue.depthStencil = {1.0f, 0};           

    VkRenderingInfo main_pass_rendering_info{};
    main_pass_rendering_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;

    main_pass_rendering_info.layerCount = 1;

    main_pass_rendering_info.colorAttachmentCount = 1;
    main_pass_rendering_info.pColorAttachments = &color_attachment;

    main_pass_rendering_info.pDepthAttachment = &depth_attachment;
    main_pass_rendering_info.pStencilAttachment = &stencil_attachment;

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

    vkCmdBindDescriptorSets(main_command_buffers[current_frame], VK_PIPELINE_BIND_POINT_GRAPHICS, main_pipeline_layout, 0, 1, &shadow_map_descriptor_sets[current_frame], 0, nullptr);

    for (const auto& object: render_objects)
    {
        VkDeviceSize offsets = 0;
        vkCmdBindVertexBuffers(main_command_buffers[current_frame], 0, 1, &object.mesh->vertex_buffer.buffer, &offsets);
        vkCmdBindIndexBuffer(main_command_buffers[current_frame], object.mesh->index_buffer.buffer, 0, VK_INDEX_TYPE_UINT16);

        glm::mat4 mvp = object.model * view_matrix * perspective_projection_matrix; //later create a camera class which owns the view matrix (push constant limited to vertex shader)

        vkCmdPushConstants(main_command_buffers[current_frame], main_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &mvp);

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

    std::vector<VkVertexInputAttributeDescription> vertex_input_attribute_descriptions = {position_input_attribute_description, color_input_attribute_description, normal_input_attribute_description};

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

    VkPushConstantRange push_constant{};
    push_constant.offset = 0;
    push_constant.size = sizeof(glm::mat4);
    push_constant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkPipelineLayoutCreateInfo pipeline_layout_info{};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 1;
    pipeline_layout_info.pSetLayouts = &set_layout_infos.at(0).layout;
    pipeline_layout_info.pushConstantRangeCount = 1;
    pipeline_layout_info.pPushConstantRanges = &push_constant;

    if (vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr, &main_pipeline_layout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create pipeline layout");
    }

    resources.push_back(ResourceTypes::PipelineLayouts, main_pipeline_layout);

    VkGraphicsPipelineCreateInfo pipeline_create_info{};
    pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
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