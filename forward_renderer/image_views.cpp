#include "image_views.hpp"

VkFormat find_supported_format(VkPhysicalDevice physical_device, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) 
{
    for (VkFormat format : candidates) 
    {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physical_device, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) return format;
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) return format;
    }
    throw std::runtime_error("failed to find supported format!");
}

VkImageView Renderer::create_swap_chain_image_view(VkImage image, VkFormat format) 
{
    VkImageViewCreateInfo view_info{};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image = image;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = format;
    view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = 1;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 1;

    VkImageView image_view;
    if (vkCreateImageView(device, &view_info, nullptr, &image_view) != VK_SUCCESS) throw std::runtime_error("failed to create image view!");

    resources.push_back(ResourceTypes::ImageViews, image_view);

    return image_view;
}

VkImageView Renderer::create_depth_image_view(VkImage image, VkFormat format) 
{
    VkImageViewCreateInfo view_info{};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image = image;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = format;
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = 1;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 1;

    VkImageView image_view;
    if (vkCreateImageView(device, &view_info, nullptr, &image_view) != VK_SUCCESS) throw std::runtime_error("failed to create image view!");

    resources.push_back(ResourceTypes::ImageViews, image_view);

    return image_view;
}

void Renderer::create_image(VkImage& image, VkFormat format, VkImageUsageFlags usage) 
{
    VkImageCreateInfo image_info{};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.extent.width = width;
    image_info.extent.height = height;
    image_info.extent.depth = 1;
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.format = format;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.usage = usage;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(device, &image_info, nullptr, &image) != VK_SUCCESS) throw std::runtime_error("failed to create image");

    VmaAllocationCreateInfo alloc_info{};
    VmaAllocation allocation;

    AllocatedImage allocated_image{};

    alloc_info.usage = VMA_MEMORY_USAGE_AUTO;

    if (vmaCreateImage(allocator, &image_info, &alloc_info, &image, &allocation, nullptr) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate images");
    }

    allocated_image.allocation = allocation;
    allocated_image.image = image;

    resources.push_back(ResourceTypes::AllocatedImages, allocated_image);
}

void Renderer::allocate_depth_images_and_views()
{
    for (auto light : directional_lights)
    {
        for (int i = 0; i < swap_chain_images.size(); i++)
        {
            create_image(light.depth_images[i], shadow_map_format, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
            light.depth_image_views[i] = create_depth_image_view(light.depth_images[i], shadow_map_format);
        }
    }
}