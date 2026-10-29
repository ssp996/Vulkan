#include "include.hpp"
#include "renderer.hpp"

void Renderer::transition_image(VkCommandBuffer cmd, VkImage image, VkImageAspectFlags aspect, VkImageLayout old_layout, VkImageLayout new_layout, VkPipelineStageFlags src_stage, VkAccessFlags src_access, VkPipelineStageFlags dst_stage, VkAccessFlags dst_access)
{
    VkImageMemoryBarrier b{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    b.srcAccessMask       = src_access;
    b.dstAccessMask       = dst_access;
    b.oldLayout           = old_layout;
    b.newLayout           = new_layout;
    b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.image               = image;
    b.subresourceRange    = { aspect, 0, 1, 0, 1 };

    vkCmdPipelineBarrier(cmd,
        src_stage, dst_stage,
        0,              // dependency flags
        0, nullptr,     // global memory barriers
        0, nullptr,     // buffer barriers
        1, &b);         // image barriers
}

void Renderer::create_semaphore_set(uint32_t n, std::vector<VkSemaphore> semaphores)
{
    semaphores.resize(n);

    VkSemaphoreCreateInfo semaphore_create_info{};
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    for (int i = 0; i < n; i++)
    {
        if (vkCreateSemaphore(device, &semaphore_create_info, nullptr, &semaphores[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create semaphores");
        }
    }
}

void Renderer::create_fence_set(uint32_t n, std::vector<VkFence>& fences, VkFenceCreateFlagBits create_flags)
{
    fences.resize(n);

    VkFenceCreateInfo fence_create_info{};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.flags = create_flags;

    for (int i = 0; i < n; i++)
    {
        if (vkCreateFence(device, &fence_create_info, nullptr, &fences[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create fences");
        }
    }
}



