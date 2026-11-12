#include "image_views.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

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

VkFormat find_depth_format(VkPhysicalDevice physical_device) 
{
    return find_supported_format(
        physical_device,
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
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
    if (vkCreateImageView(device, &view_info, nullptr, &image_view) != VK_SUCCESS) throw std::runtime_error("failed to create image view");

    resources.push_back(ResourceTypes::ImageViews, image_view);

    return image_view;
}

void Renderer::create_image(VkImage& image, VkFormat format, VkImageUsageFlags usage, VkExtent2D extent) 
{
    VkImageCreateInfo image_info{};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.extent.width = extent.width;
    image_info.extent.height = extent.height;
    image_info.extent.depth = 1;
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.format = format;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.usage = usage;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;


    VmaAllocationCreateInfo alloc_info{};
    VmaAllocation allocation;

    alloc_info.usage = VMA_MEMORY_USAGE_AUTO;

    if (vmaCreateImage(allocator, &image_info, &alloc_info, &image, &allocation, nullptr) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate images");
    }

    AllocatedImage allocated_image{};
    allocated_image.allocation = allocation;
    allocated_image.image = image;

    resources.push_back(ResourceTypes::AllocatedImages, allocated_image);
}

void Renderer::create_cubemap(VkImage& image, VkExtent2D face_extent)
{
    VkImageCreateInfo image_info{};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.extent = { cubemap_face_extent.width, cubemap_face_extent.height, 1 };
    image_info.mipLevels = 1;
    image_info.arrayLayers = 6; 
    image_info.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT; 
    image_info.format = shadow_map_format;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo alloc_info{};
    VmaAllocation allocation;

    alloc_info.usage = VMA_MEMORY_USAGE_AUTO;

    if (vmaCreateImage(allocator, &image_info, &alloc_info, &image, &allocation, nullptr) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate images");
    }

    AllocatedImage allocated_image{};
    allocated_image.allocation = allocation;
    allocated_image.image = image;

    resources.push_back(ResourceTypes::AllocatedImages, allocated_image);
}

VkImageView Renderer::create_cubemap_views(VkImage image)
{
    VkImageViewCreateInfo view_info{};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image = image;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    view_info.format = shadow_map_format;
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = 1;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 6;

    VkImageView image_view;
    if (vkCreateImageView(device, &view_info, nullptr, &image_view) != VK_SUCCESS) throw std::runtime_error("failed to create image view");

    resources.push_back(ResourceTypes::ImageViews, image_view);

    return image_view;   
}

void Renderer::allocate_shadow_map_images_and_views()
{
    for (auto& light : directional_lights)
    {
        for (int i = 0; i < swap_chain_images.size(); i++)
        {
            create_image(light.depth_images[i], shadow_map_format, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, directional_shadow_map_extent);
            light.depth_image_views[i] = create_depth_image_view(light.depth_images[i], shadow_map_format);
        }
    }
}

void Renderer::allocate_cubemap_images_and_views()
{
    for (auto& light: point_lights)
    {
        for (int i = 0; i < swap_chain_images.size(); i++)
        {
            create_cubemap(light.depth_images[i], cubemap_face_extent);      
            light.depth_image_views[i] = create_cubemap_views(light.depth_images[i]);
        }
    }
}

void Renderer::allocate_depth_image_views()
{
    VkFormat depth_format = find_depth_format(physical_device);

    depth_images.resize(swap_chain_images.size());
    depth_image_views.resize(swap_chain_images.size());

    for (int i = 0; i < swap_chain_images.size(); i++)
    {
        create_image(depth_images[i], depth_format, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, swap_chain_extent);
        depth_image_views[i] = create_depth_image_view(depth_images[i], depth_format);
    }
}

void Renderer::load_texture(const std::string& filepath, AllocatedImage& out_image, VkImageView& out_view)
{
    int tex_width, tex_height, tex_channels;
    stbi_uc* pixels = stbi_load(filepath.c_str(), &tex_width, &tex_height, &tex_channels, STBI_rgb_alpha);

    VkDeviceSize image_size = tex_width * tex_height * 4;

    if (!pixels)
    {
        throw std::runtime_error("failed to open texture image");
    }

    AllocatedBuffer pixels_staging_buffer = create_staging_buffer((void*)pixels, image_size);
    stbi_image_free(pixels);

    VkExtent2D extent = {static_cast<uint32_t>(tex_width), static_cast<uint32_t>(tex_height)};
    create_image(out_image.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, extent);

    VkCommandBuffer temp_cmd;


    begin_temp_command_buffer(temp_cmd);

    transition_image(temp_cmd, out_image.image, VK_IMAGE_ASPECT_COLOR_BIT, 
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, VK_ACCESS_2_NONE,
        VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT);

    copy_buffer_to_image(pixels_staging_buffer.buffer, out_image.image, static_cast<uint32_t>(tex_width), static_cast<uint32_t>(tex_height), temp_cmd);

     transition_image(temp_cmd, out_image.image, VK_IMAGE_ASPECT_COLOR_BIT, 
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
        VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT);

    end_temp_command_buffer(temp_cmd);

    vmaDestroyBuffer(allocator, pixels_staging_buffer.buffer, pixels_staging_buffer.allocation);

    out_view = create_swap_chain_image_view(out_image.image, VK_FORMAT_R8G8B8A8_SRGB);
}   

void Renderer::create_texture_sampler()
{
    VkSamplerCreateInfo sampler_info{};
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.magFilter = VK_FILTER_LINEAR;
    sampler_info.minFilter = VK_FILTER_LINEAR;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.anisotropyEnable = VK_FALSE;
    sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler_info.unnormalizedCoordinates = VK_FALSE;
    sampler_info.compareEnable = VK_FALSE;
    sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    if (vkCreateSampler(device, &sampler_info, nullptr, &texture_sampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }
    resources.push_back(ResourceTypes::Samplers, texture_sampler);
}