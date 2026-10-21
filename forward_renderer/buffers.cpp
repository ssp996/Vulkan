#include "renderer.hpp"

void Renderer::create_buffer(size_t size)
{
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(physical_device, &properties);
    size_t minAlignment = properties.limits.minUniformBufferOffsetAlignment;

    
}