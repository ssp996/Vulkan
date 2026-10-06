#include "renderer.hpp"

void Renderer::directional_shadow_pass(uint32_t current_frame)
{
    for (int i = 0; i < directional_lights.size(); i++)
    {
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

    vkDestroyShaderModule(device, vert_module, nullptr);
}