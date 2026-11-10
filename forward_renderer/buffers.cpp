#include "renderer.hpp"

void Renderer::copy_buffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size)
{   
    VkCommandBuffer temp_cmd;
    begin_temp_command_buffer(temp_cmd);

    VkBufferCopy copy_region{};
    copy_region.size = size;
    vkCmdCopyBuffer(temp_cmd, src_buffer, dst_buffer, 1, &copy_region);

    end_temp_command_buffer(temp_cmd);
}

AllocatedBuffer Renderer::create_staging_buffer(void* data, size_t size)
{
    VkBuffer staging_buffer;
    VmaAllocation staging_allocation;
    
    VkBufferCreateInfo staging_info{};
    staging_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    staging_info.size = (VkDeviceSize)size;
    staging_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT; 
    
    VmaAllocationCreateInfo staging_alloc_info{};
    staging_alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
    staging_alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
    
    VmaAllocationInfo alloc_info;
    vmaCreateBuffer(allocator, &staging_info, &staging_alloc_info, &staging_buffer, &staging_allocation, &alloc_info);

    memcpy(alloc_info.pMappedData, data, size);
    
    AllocatedBuffer asb{};
    asb.buffer = staging_buffer;
    asb.allocation = staging_allocation;
    return asb;
}

void Renderer::copy_buffer_to_image(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, VkCommandBuffer cmd_buf)
{
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};

    vkCmdCopyBufferToImage(cmd_buf, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

}

void Renderer::create_and_map_descriptor_buffer(size_t size,  uint32_t n_elements, VkBufferUsageFlags buffer_usage, AllocatedBuffer& buffer)
{
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(physical_device, &properties);
    size_t minAlignment = properties.limits.minUniformBufferOffsetAlignment;

    size_t padded_size = size;

    if (minAlignment > 0)
    {
        padded_size = (padded_size + minAlignment - 1) & ~(minAlignment - 1);
    }

    VkBufferCreateInfo buffer_info{};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = padded_size * n_elements;
    buffer_info.usage = buffer_usage;

    VmaAllocationCreateInfo alloc_create_info{};
    alloc_create_info.usage = VMA_MEMORY_USAGE_AUTO;
    alloc_create_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VmaAllocationInfo allocation_info;

    if (vmaCreateBuffer(allocator, &buffer_info, &alloc_create_info, &buffer.buffer, &buffer.allocation, &allocation_info) != VK_SUCCESS)
    {
        throw std::runtime_error ("failed to create buffer");
    }

    buffer.mapped = allocation_info.pMappedData;
    buffer.padded_size = padded_size;
    buffer.elem_size = size;

    resources.push_back(ResourceTypes::AllocatedBuffer, buffer);
}

void Renderer::create_staging_vertex_buffer(const std::vector<Vertex>& vertices, const std::vector<uint16_t>& indices, Mesh& mesh)
{
    //gpu buffer so cant be mapped
    mesh.vertex_buffer.mapped = nullptr;
    mesh.index_buffer.mapped = nullptr;

    AllocatedBuffer vertex_staging_buffer = create_staging_buffer((void*)vertices.data(), sizeof(Vertex) * vertices.size());

    VkDeviceSize vertex_buffer_size = (VkDeviceSize)(sizeof(Vertex) * vertices.size());

    VkBufferCreateInfo gpu_info{};
    gpu_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    gpu_info.size = vertex_buffer_size;
    gpu_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT; 
    
    VmaAllocationCreateInfo gpu_alloc_info{};
    gpu_alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE; 
    
    vmaCreateBuffer(allocator, &gpu_info, &gpu_alloc_info, &mesh.vertex_buffer.buffer, &mesh.vertex_buffer.allocation, nullptr);
    copy_buffer(vertex_staging_buffer.buffer, mesh.vertex_buffer.buffer, vertex_buffer_size);
    vmaDestroyBuffer(allocator, vertex_staging_buffer.buffer, vertex_staging_buffer.allocation);

    resources.push_back(ResourceTypes::AllocatedBuffer, mesh.vertex_buffer);

    AllocatedBuffer index_staging_buffer = create_staging_buffer((void*)indices.data(), sizeof(uint16_t) * indices.size());

    VkDeviceSize index_buffer_size = (VkDeviceSize)(sizeof(uint16_t) * indices.size());

    gpu_info.size = index_buffer_size;
    gpu_info.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    vmaCreateBuffer(allocator, &gpu_info, &gpu_alloc_info, &mesh.index_buffer.buffer, &mesh.index_buffer.allocation, nullptr);
    copy_buffer(index_staging_buffer.buffer, mesh.index_buffer.buffer, index_buffer_size);
    vmaDestroyBuffer(allocator, index_staging_buffer.buffer, index_staging_buffer.allocation);
    
    resources.push_back(ResourceTypes::AllocatedBuffer, mesh.index_buffer);
}

void Renderer::create_and_map_vertex_buffer(const std::vector<Vertex>& vertices, std::vector<uint16_t> indices, Mesh& mesh)
{
    VkBufferCreateInfo buffer_create_info{};
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.size = sizeof(vertices[0]) * vertices.size();
    buffer_create_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo alloc_create_info{};
    alloc_create_info.usage = VMA_MEMORY_USAGE_AUTO;
    alloc_create_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VmaAllocationInfo allocation_info;

    if (vmaCreateBuffer(allocator, &buffer_create_info, &alloc_create_info, &mesh.vertex_buffer.buffer, &mesh.vertex_buffer.allocation, &allocation_info) != VK_SUCCESS)
    {
        throw std::runtime_error ("failed to create buffer");
    }

    mesh.vertex_buffer.mapped = allocation_info.pMappedData;

    std::byte* dst = static_cast<std::byte*>(mesh.vertex_buffer.mapped);
    memcpy(dst, vertices.data(), sizeof(vertices[0]) * vertices.size());

    resources.push_back(ResourceTypes::AllocatedBuffer, mesh.vertex_buffer);

    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.size = sizeof(indices[0]) * indices.size();
    buffer_create_info.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    alloc_create_info.usage = VMA_MEMORY_USAGE_AUTO;
    alloc_create_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
    
    if (vmaCreateBuffer(allocator, &buffer_create_info, &alloc_create_info, &mesh.index_buffer.buffer, &mesh.index_buffer.allocation, &allocation_info) != VK_SUCCESS)
    {
        throw std::runtime_error ("failed to create buffer");
    }

    mesh.index_buffer.mapped = allocation_info.pMappedData;

    dst = static_cast<std::byte*>(mesh.index_buffer.mapped);
    memcpy(dst, indices.data(), sizeof(indices[0]) * indices.size());

    resources.push_back(ResourceTypes::AllocatedBuffer, mesh.index_buffer);
}