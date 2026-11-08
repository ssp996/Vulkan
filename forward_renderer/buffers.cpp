#include "renderer.hpp"

void Renderer::copy_buffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size)
{
    VkCommandBufferAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandPool = command_pool;
    alloc_info.commandBufferCount = 1;

    VkCommandBuffer temp_cmd;
    vkAllocateCommandBuffers(device, &alloc_info, &temp_cmd);

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(temp_cmd, &begin_info);

    VkBufferCopy copy_region{};
    copy_region.size = size;
    vkCmdCopyBuffer(temp_cmd, src_buffer, dst_buffer, 1, &copy_region);

    vkEndCommandBuffer(temp_cmd);

    VkCommandBufferSubmitInfo cmd_submit{};
    cmd_submit.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    cmd_submit.commandBuffer = temp_cmd;

    VkSubmitInfo2 submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    submit.commandBufferInfoCount = 1;
    submit.pCommandBufferInfos = &cmd_submit;

    vkQueueSubmit2(graphics_queue, 1, &submit, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphics_queue);

    vkFreeCommandBuffers(device, command_pool, 1, &temp_cmd);
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

    create_staging_buffer<Vertex>(vertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, mesh.vertex_buffer);
    create_staging_buffer<uint16_t>(indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, mesh.index_buffer);

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

/* void Renderer::create_directional_light_buffer_objects(uint32_t n)
{
    for (int i = 0; i < n; i++)
    {
        lbos[i].active_lights = static_cast<int>(directional_lights.size());
        lbos[i].camera_pos = camera_pos;

        for (int i = 0; i < MAX_DIRECTIONAL_LIGHTS; i++)
        {
            if (i < directional_lights.size())
            {
                lbos[i].dir_lights[i] = directional_lights[i].to_gpu();
            }
            else
            { 
                lbos[i].dir_lights[i] = DirLight{};
            }
        }
    }
} */