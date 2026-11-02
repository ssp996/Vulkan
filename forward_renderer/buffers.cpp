#include "renderer.hpp"

void Renderer::create_and_map_buffer(size_t size,  uint32_t n_elements, VkBufferUsageFlags buffer_usage, AllocatedBuffer& buffer)
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
    buffer.mapped = allocation_info.pMappedData;

    if (vmaCreateBuffer(allocator, &buffer_info, &alloc_create_info, &buffer.buffer, &buffer.allocation, &allocation_info) != VK_SUCCESS)
    {
        throw std::runtime_error ("failed to create buffer");
    }

    resources.push_back(ResourceTypes::AllocatedBuffer, buffer);
}

void Renderer::create_directional_light_buffer_objects(uint32_t n)
{
    for (int i = 0; i < n; i++)
    {
        LightBufferObject lbo{};
        lbo.active_lights = static_cast<int>(directional_lights.size());
        lbo.camera_pos = camera_pos;

        for (int i = 0; i < MAX_DIRECTIONAL_LIGHTS; i++)
        {
            if (i < directional_lights.size())
            {
                lbo.dir_lights[i] = directional_lights[i].to_gpu();
            }
            else
            { 
                lbo.dir_lights[i] = DirLight{};
            }
        }
    }
}