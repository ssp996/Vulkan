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

void Renderer::main_pass(uint32_t current_frame)
{
    VkRenderingAttachmentInfo color_attachment{};
    
    color_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;

    color_attachment.clearValue.color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    color_attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color_attachment.imageView = swap_chain_image_views[current_frame];

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

    main_pass_rendering_info.renderArea.extent = swap_chain_extent;
    main_pass_rendering_info.renderArea.offset = {0, 0};

    vkCmdBeginRendering(main_command_buffers[current_frame], &main_pass_rendering_info);

    vkCmdBindPipeline(main_command_buffers[current_frame], VK_PIPELINE_BIND_POINT_GRAPHICS, directional_shadow_pipeline);

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

    vkCmdBindDescriptorSets(main_command_buffers[current_frame], VK_PIPELINE_BIND_POINT_GRAPHICS, main_pipeline_layout, 0, 1, shadow_map_descriptor_sets.data(), 0, nullptr);

    for (const auto& object: render_objects)
    {
        VkDeviceSize offsets = 0;
        vkCmdBindVertexBuffers(main_command_buffers[current_frame], 0, 1, &object.mesh->vertex_buffer.buffer, &offsets);
        vkCmdBindIndexBuffer(main_command_buffers[current_frame], object.mesh->index_buffer.buffer, 0, VK_INDEX_TYPE_UINT16);

        glm::mat4 mvp = object.model * view_matrix * perspective_projection_matrix; //later create a camera class which owns the view matrix

        vkCmdPushConstants(main_command_buffers[current_frame], main_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &mvp);

        vkCmdDrawIndexed(main_command_buffers[current_frame], object.mesh->index_count, 1, 0, 0, 0);  
    }

    vkCmdEndRendering(main_command_buffers[current_frame]);

}