#include "renderer.hpp"

void Renderer::directional_shadow_pass(uint32_t current_frame)
{

    vkCmdBindPipeline(main_command_buffers[current_frame], VK_PIPELINE_BIND_POINT_GRAPHICS, directional_shadow_pipeline);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(directional_shadow_map_extent.width);
    viewport.height = static_cast<float>(directional_shadow_map_extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(main_command_buffers[current_frame], 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = directional_shadow_map_extent;
    vkCmdSetScissor(main_command_buffers[current_frame], 0, 1, &scissor);

    float depthBiasConstant = 1.25f;
    float depthBiasSlope = 1.75f;
    vkCmdSetDepthBias(main_command_buffers[current_frame], depthBiasConstant, 0.0f, depthBiasSlope);

    for (int i = 0; i < directional_lights.size(); i++)
    {
        if (directional_lights.empty()) return;

        VkRenderingAttachmentInfo depth_attachment{};

        depth_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;

        depth_attachment.clearValue.depthStencil = {1.0f, 0};
        depth_attachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
        depth_attachment.imageView = directional_lights[i].depth_image_views[current_frame];

        depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        
        VkRenderingInfo directional_shadow_pass_rendering_info{};
        directional_shadow_pass_rendering_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        directional_shadow_pass_rendering_info.renderArea.extent = directional_shadow_map_extent;
        directional_shadow_pass_rendering_info.renderArea.offset = {0, 0};
        directional_shadow_pass_rendering_info.layerCount = 1;
        directional_shadow_pass_rendering_info.colorAttachmentCount = 0;
        directional_shadow_pass_rendering_info.pDepthAttachment = &depth_attachment;

        vkCmdBeginRendering(main_command_buffers[current_frame], &directional_shadow_pass_rendering_info);
    
        for (int j = 0; j < render_objects.size(); j++)
        {
            VkDeviceSize offsets = 0;
            vkCmdBindVertexBuffers(main_command_buffers[current_frame], 0, 1, &render_objects[j].mesh->vertex_buffer.buffer, &offsets);
            vkCmdBindIndexBuffer(main_command_buffers[current_frame], render_objects[j].mesh->index_buffer.buffer, 0, VK_INDEX_TYPE_UINT16);

            DirectionalLightPushConstant pc = {render_objects[j].model, directional_lights[i].light_space_matrix};

            vkCmdPushConstants(main_command_buffers[current_frame], directional_shadow_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(DirectionalLightPushConstant), &pc);
            
            vkCmdDrawIndexed(main_command_buffers[current_frame], render_objects[j].mesh->index_count, 1, 0, 0, 0);  
        }

        vkCmdEndRendering(main_command_buffers[current_frame]);
    }
}

void Renderer::create_shadow_pipeline()
{
    std::vector<char> vert_code = readFile(directional_shadow_vertex_shader_filepath);

    VkShaderModule vert_module = createShaderModule(vert_code, device);

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

    rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT; 
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE; 

    rasterizer.depthBiasEnable = VK_TRUE;
    rasterizer.depthBiasConstantFactor = 1.25f; 
    rasterizer.depthBiasSlopeFactor = 1.75f;    
    rasterizer.depthBiasClamp = 0.0f;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    std::vector<VkDynamicState> dynamic_states = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_DEPTH_BIAS};
    VkPipelineDynamicStateCreateInfo dynamic_state{};
    dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
    dynamic_state.pDynamicStates = dynamic_states.data();

    VkPushConstantRange push_constant{};
    push_constant.offset = 0;
    push_constant.size = static_cast<uint32_t>(sizeof(DirectionalLightPushConstant));
    push_constant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkPipelineLayoutCreateInfo pipeline_layout_info{};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 0;
    pipeline_layout_info.pSetLayouts = nullptr;
    pipeline_layout_info.pushConstantRangeCount = 1;
    pipeline_layout_info.pPushConstantRanges = &push_constant;

    if (vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr, &directional_shadow_pipeline_layout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout");
    }

    resources.push_back(ResourceTypes::PipelineLayouts, directional_shadow_pipeline_layout);

    VkPipelineShaderStageCreateInfo vert_stage[] = {
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_VERTEX_BIT, vert_module, "main", nullptr}
    };

    VkVertexInputBindingDescription binding_desc{};
    binding_desc.binding = 0;
    binding_desc.stride = sizeof(Vertex);
    binding_desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attributeDescription{};
    attributeDescription.binding = 0;
    attributeDescription.location = 0; 
    attributeDescription.format = VK_FORMAT_R32G32B32_SFLOAT; 
    attributeDescription.offset = offsetof(Vertex, position);

    VkPipelineVertexInputStateCreateInfo vertex_input{};
    vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input.vertexBindingDescriptionCount = 1;
    vertex_input.pVertexBindingDescriptions = &binding_desc;
    vertex_input.vertexAttributeDescriptionCount = 1;
    vertex_input.pVertexAttributeDescriptions = &attributeDescription;

    VkPipelineInputAssemblyStateCreateInfo input_assembly{};
    input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; 

    VkPipelineDepthStencilStateCreateInfo depth_stencil_create_info{};
    depth_stencil_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil_create_info.depthTestEnable = VK_TRUE;           
    depth_stencil_create_info.depthWriteEnable = VK_TRUE;          
    depth_stencil_create_info.depthCompareOp = VK_COMPARE_OP_LESS;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 0; 
    colorBlending.pAttachments = nullptr;

    VkPipelineRenderingCreateInfo pipeline_rendering_create_info{};
    pipeline_rendering_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    pipeline_rendering_create_info.colorAttachmentCount = 0;
    pipeline_rendering_create_info.depthAttachmentFormat = shadow_map_format;
    pipeline_rendering_create_info.pColorAttachmentFormats = nullptr;
    pipeline_rendering_create_info.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;


    VkGraphicsPipelineCreateInfo pipeline_info{};
    pipeline_info.pNext = &pipeline_rendering_create_info;
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = 1;
    pipeline_info.pStages = vert_stage;
    pipeline_info.pVertexInputState = &vertex_input;
    pipeline_info.pInputAssemblyState = &input_assembly;
    pipeline_info.pViewportState = &viewport_state;
    pipeline_info.pRasterizationState = &rasterizer;
    pipeline_info.pMultisampleState = &multisampling;
    pipeline_info.pDepthStencilState = &depth_stencil_create_info;
    pipeline_info.pColorBlendState = &colorBlending;
    pipeline_info.pDynamicState = &dynamic_state;
    pipeline_info.layout = directional_shadow_pipeline_layout;
    pipeline_info.renderPass = VK_NULL_HANDLE;
    pipeline_info.subpass = 0; 

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &directional_shadow_pipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create directional shadow pipeline");
    }

    resources.push_back(ResourceTypes::Pipelines, directional_shadow_pipeline);

    vkDestroyShaderModule(device, vert_module, nullptr);
}

void Renderer::create_shadow_sampler()
{
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;

    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE; 
 
    samplerInfo.compareEnable = VK_TRUE;
    samplerInfo.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

    if (vkCreateSampler(device, &samplerInfo, nullptr, &shadow_sampler) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create shadow sampler");
    }

    resources.push_back(ResourceTypes::Samplers, shadow_sampler);
}

void Renderer::create_point_light_matrices_descriptor_sets()
{   
    std::cout << "accessing pointlight layout" << std::endl;
    const SetLayoutInfo& set_layout_info = point_shadow_pipeline_set_layout_infos.at(0);
    point_light_matrices_descriptor_sets.resize(point_lights.size());
    point_light_matrices_buffers.resize(point_lights.size());

    for (int i = 0; i < point_lights.size(); i++)
    {
        allocate_descriptor_set(set_layout_info.layout, point_light_matrices_descriptor_sets[i], static_cast<uint32_t>(swap_chain_images.size()));

        create_and_map_descriptor_buffer(6 * sizeof(glm::mat4), swap_chain_images.size(), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, point_light_matrices_buffers[i]);

        for (int j = 0; j < swap_chain_images.size(); j++)
        {
            update_point_light_matrices_buffer(j, i, point_light_matrices_buffers[i]);
        }

        for (int frame = 0; frame < swap_chain_images.size(); frame++)
        {
            BindingData matrix_bd{};
            matrix_bd.name = "point_light_matrices";
            matrix_bd.buffers = {make_buffer_info(point_light_matrices_buffers[i], frame)};
           

            std::vector<BindingData> binding_data = {matrix_bd};
            write_set(point_light_matrices_descriptor_sets[i][frame], set_layout_info, binding_data);
        }
    }     
}

void Renderer::update_point_light_matrices_buffer(uint32_t current_frame, uint32_t light_index, AllocatedBuffer& point_light_matrices_buffer)
{
    std::vector<glm::mat4> light_matrices = point_lights[light_index].get_matrices();

    VkDeviceSize offset = current_frame * point_light_matrices_buffer.padded_size;
    std::byte* dst = static_cast<std::byte*>(point_light_matrices_buffer.mapped) + offset;

    assert(point_light_matrices_buffer.mapped != nullptr && "VMA memory was never mapped for point light matrices buffer");

    memcpy(dst, light_matrices.data(), 6 * sizeof(glm::mat4));
    vmaFlushAllocation(allocator, point_light_matrices_buffer.allocation, offset, point_light_matrices_buffer.padded_size);
}

void Renderer::point_shadow_pass(uint32_t current_frame)
{
    vkCmdBindPipeline(main_command_buffers[current_frame], VK_PIPELINE_BIND_POINT_GRAPHICS, point_shadow_pipeline);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(cubemap_face_extent.width);
    viewport.height = static_cast<float>(cubemap_face_extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(main_command_buffers[current_frame], 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = cubemap_face_extent;
    vkCmdSetScissor(main_command_buffers[current_frame], 0, 1, &scissor);

    float depthBiasConstant = 1.25f;
    float depthBiasSlope = 1.75f;
    vkCmdSetDepthBias(main_command_buffers[current_frame], depthBiasConstant, 0.0f, depthBiasSlope); 
    
    for (int i = 0; i < point_lights.size(); i++)
    {
        if (point_lights.empty()) return;

        VkRenderingAttachmentInfo depth_attachment{};
        depth_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        depth_attachment.imageView = point_lights[i].depth_image_views[current_frame];
        depth_attachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
        depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        depth_attachment.clearValue.depthStencil = {1.0f, 0};

        VkRenderingInfo render_info{};
        render_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        render_info.renderArea.extent = cubemap_face_extent;
        render_info.layerCount = 1; 
        render_info.viewMask = 0x3F; 
        render_info.pDepthAttachment = &depth_attachment;
        
        vkCmdBeginRendering(main_command_buffers[current_frame], &render_info);

        vkCmdBindDescriptorSets(main_command_buffers[current_frame], VK_PIPELINE_BIND_POINT_GRAPHICS, point_shadow_pipeline_layout, 0, 1, &point_light_matrices_descriptor_sets[i][current_frame], 0, nullptr);
        for (int j = 0; j < render_objects.size(); j++)
        {
            VkDeviceSize offsets = 0;
            vkCmdBindVertexBuffers(main_command_buffers[current_frame], 0, 1, &render_objects[j].mesh->vertex_buffer.buffer, &offsets);
            vkCmdBindIndexBuffer(main_command_buffers[current_frame], render_objects[j].mesh->index_buffer.buffer, 0, VK_INDEX_TYPE_UINT16);
            
            glm::mat4 pc = render_objects[j].model;

            vkCmdPushConstants(main_command_buffers[current_frame], point_shadow_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &pc);

            vkCmdDrawIndexed(main_command_buffers[current_frame], render_objects[j].mesh->index_count, 1, 0, 0, 0);
        }

        vkCmdEndRendering(main_command_buffers[current_frame]);
    }
}

void Renderer::create_point_shadow_pipeline()
{
    std::vector<char> vert_code = readFile(point_shadow_vertex_shader_filepath);

    VkShaderModule vert_module = createShaderModule(vert_code, device);
    
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

    rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT; 
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE; 

    rasterizer.depthBiasEnable = VK_TRUE;
    rasterizer.depthBiasConstantFactor = 1.25f; 
    rasterizer.depthBiasSlopeFactor = 1.75f;    
    rasterizer.depthBiasClamp = 0.0f;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    std::vector<VkDynamicState> dynamic_states = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_DEPTH_BIAS};
    VkPipelineDynamicStateCreateInfo dynamic_state{};
    dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
    dynamic_state.pDynamicStates = dynamic_states.data();

    VkPushConstantRange push_constant{};
    push_constant.offset = 0;
    push_constant.size = static_cast<uint32_t>(sizeof(glm::mat4));
    push_constant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkPipelineLayoutCreateInfo pipeline_layout_info{};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 1;
    std::cout << "assigning point light layout to pipeline" << std::endl;
    pipeline_layout_info.pSetLayouts = &point_shadow_pipeline_set_layout_infos.at(0).layout;
    pipeline_layout_info.pushConstantRangeCount = 1;
    pipeline_layout_info.pPushConstantRanges = &push_constant;

    if (vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr, &point_shadow_pipeline_layout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout");
    }

    resources.push_back(ResourceTypes::PipelineLayouts, point_shadow_pipeline_layout);

    VkPipelineShaderStageCreateInfo vert_stage[] = {
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_VERTEX_BIT, vert_module, "main", nullptr}
    };

     VkVertexInputBindingDescription binding_desc{};
    binding_desc.binding = 0;
    binding_desc.stride = sizeof(Vertex);
    binding_desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attributeDescription{};
    attributeDescription.binding = 0;
    attributeDescription.location = 0; 
    attributeDescription.format = VK_FORMAT_R32G32B32_SFLOAT; 
    attributeDescription.offset = offsetof(Vertex, position);

    VkPipelineVertexInputStateCreateInfo vertex_input{};
    vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input.vertexBindingDescriptionCount = 1;
    vertex_input.pVertexBindingDescriptions = &binding_desc;
    vertex_input.vertexAttributeDescriptionCount = 1;
    vertex_input.pVertexAttributeDescriptions = &attributeDescription;

    VkPipelineInputAssemblyStateCreateInfo input_assembly{};
    input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; 

    VkPipelineDepthStencilStateCreateInfo depth_stencil_create_info{};
    depth_stencil_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil_create_info.depthTestEnable = VK_TRUE;           
    depth_stencil_create_info.depthWriteEnable = VK_TRUE;          
    depth_stencil_create_info.depthCompareOp = VK_COMPARE_OP_LESS;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 0; 
    colorBlending.pAttachments = nullptr;

    VkPipelineRenderingCreateInfo pipeline_rendering_create_info{};
    pipeline_rendering_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    pipeline_rendering_create_info.colorAttachmentCount = 0;
    pipeline_rendering_create_info.depthAttachmentFormat = shadow_map_format;
    pipeline_rendering_create_info.pColorAttachmentFormats = nullptr;
    pipeline_rendering_create_info.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;
    pipeline_rendering_create_info.viewMask = 0x3F;

    VkGraphicsPipelineCreateInfo pipeline_info{};
    pipeline_info.pNext = &pipeline_rendering_create_info;
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = 1;
    pipeline_info.pStages = vert_stage;
    pipeline_info.pVertexInputState = &vertex_input;
    pipeline_info.pInputAssemblyState = &input_assembly;
    pipeline_info.pViewportState = &viewport_state;
    pipeline_info.pRasterizationState = &rasterizer;
    pipeline_info.pMultisampleState = &multisampling;
    pipeline_info.pDepthStencilState = &depth_stencil_create_info;
    pipeline_info.pColorBlendState = &colorBlending;
    pipeline_info.pDynamicState = &dynamic_state;
    pipeline_info.layout = point_shadow_pipeline_layout;
    pipeline_info.renderPass = VK_NULL_HANDLE;
    pipeline_info.subpass = 0; 

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &point_shadow_pipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create point shadow pipeline");
    }

    resources.push_back(ResourceTypes::Pipelines, point_shadow_pipeline);

    vkDestroyShaderModule(device, vert_module, nullptr);
}