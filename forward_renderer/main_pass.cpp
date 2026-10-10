#include "renderer.hpp"

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

    
}