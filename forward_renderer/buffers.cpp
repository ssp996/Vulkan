#include "renderer.hpp"

void Renderer::create_and_map_buffer(size_t size,  uint32_t n_elements, VkBufferUsageFlags buffer_usage)
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

    if (vmaCreateBuffer(allocator, &buffer_info, &alloc_create_info, &dir_lights_buffer.buffer, &dir_lights_buffer.allocation, &allocation_info) != VK_SUCCESS)
    {
        throw std::runtime_error ("failed to create buffer");
    }

    resources.push_back(ResourceTypes::AllocatedBuffer, dir_lights_buffer);
}

void create_directional_light_buffer_objects(const std::vector<DirectionalLight>& directional_lights, uint32_t n, std::vector<LightBufferObject> directional_light_buffer_objects)
{
    directional_light_buffer_objects.resize(n);
    for (int i = 0; i < n; i++)
    {   
       LightBufferObject lbo{};
        lbo.active_lights = static_cast<int>(directional_lights.size());

        for (int i = 0; i < MAX_DIRECTIONAL_LIGHTS; i++)
        {
            if (i < directional_lights.size())
            {
                lbo.dir_lights[i].color = directional_lights[i].color;
                lbo.dir_lights[i].direction = directional_lights[i].direction;
                lbo.dir_lights[i].intensity = directional_lights[i].intensity;
                lbo.dir_lights[i].light_space_matrix = directional_lights[i].light_space_matrix;
            }
            else
            { 
                lbo.dir_lights[i] = DirLight{};
            }
        }
    }
}