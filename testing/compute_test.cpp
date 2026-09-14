#include <iostream>
#include <stdexcept>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "vulkan_utils_testing.hpp"
#include "vulkan_compute.hpp"

struct PushConstantFactor{
    alignas(8) float f;
};

int main()
{
    VkInstance instance;
    create_instance(instance, "compute_test", VK_API_VERSION_1_4);

    VkDebugUtilsMessengerEXT debug_messenger;
    setupDebugMessenger(instance, debug_messenger);

    VkPhysicalDevice physical_device;
    pick_compute_physical_device(instance, physical_device);

    VkDevice device;
    VkQueue compute_queue;
    create_compute_device(physical_device, device, compute_queue);

    VkDescriptorSetLayout compute_set_layout;

    create_custom_descriptor_set_layout(device, compute_set_layout, {0}, {1}, {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER}, {nullptr}, {VK_SHADER_STAGE_COMPUTE_BIT});

    VkDescriptorPool compute_pool;
    create_custom_descriptor_pool({VK_DESCRIPTOR_TYPE_STORAGE_BUFFER}, {1}, 1, compute_pool, device);

    std::vector<float> items(128);
    for (int i = 0; i < 128; i++)
    {
        items[i] = static_cast<float>(i + 1);
    }

    VkBuffer buffer;
    VkDeviceMemory buffer_memory;

    create_buffer<float>(device, physical_device, items, buffer, buffer_memory, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE);

    VkDescriptorSet compute_set;

    VkDescriptorSetAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = compute_pool;
    alloc_info.descriptorSetCount = 1;
    alloc_info.pSetLayouts = &compute_set_layout;

    if (vkAllocateDescriptorSets(device, &alloc_info, &compute_set) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate descriptor sets");
    }

    VkDescriptorBufferInfo buffer_info{};
    buffer_info.buffer = buffer;
    buffer_info.offset = 0;
    buffer_info.range = sizeof(float) * items.size();

    VkWriteDescriptorSet descriptor_write{};
    descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_write.dstSet = compute_set;
    descriptor_write.dstBinding = 0;
    descriptor_write.dstArrayElement = 0;
    descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptor_write.descriptorCount = 1;
    descriptor_write.pBufferInfo = &buffer_info;

    vkUpdateDescriptorSets(device, 1, &descriptor_write, 0, nullptr);

    VkPipelineLayout compute_pipeline_layout;
    VkPipeline compute_pipeline;

    std::vector<VkPushConstantRange> push_constant_ranges = get_push_constant_ranges<PushConstantData>({VK_SHADER_STAGE_COMPUTE_BIT});

    create_compute_pipeline("shaders/compute_test.spv", compute_pipeline_layout, compute_pipeline, {compute_set_layout}, device, push_constant_ranges);

    VkCommandPool command_pool;

    VkCommandPoolCreateInfo command_pool_create_info{};
    command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;

    int compute_queue_index = is_compute_ready(physical_device);
    if (compute_queue_index == -1)
    {  
        throw std::runtime_error("failed to find compute_queue");
    }
    command_pool_create_info.queueFamilyIndex = static_cast<uint32_t>(compute_queue_index);

    if (vkCreateCommandPool(device, &command_pool_create_info, nullptr, &command_pool) != VK_SUCCESS) 
    {
        throw std::runtime_error("failed to create command pool");
    }

    std::vector<VkCommandBuffer> command_buffers(1);

    create_command_buffers(command_pool, device, command_buffers);

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; 

    if (vkBeginCommandBuffer(command_buffers[0], &begin_info) != VK_SUCCESS) 
    {
        throw std::runtime_error("failed to begin recording command buffer");
    }

    vkCmdBindPipeline(command_buffers[0], VK_PIPELINE_BIND_POINT_COMPUTE, compute_pipeline);

    vkCmdBindDescriptorSets(command_buffers[0], VK_PIPELINE_BIND_POINT_COMPUTE, compute_pipeline_layout, 0, 1, &compute_set, 0, nullptr);

    float my_factor = 2.0f; 
    vkCmdPushConstants(command_buffers[0], compute_pipeline_layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(float), &my_factor);

    vkCmdDispatch(command_buffers[0], 1, 1, 1);

    if (vkEndCommandBuffer(command_buffers[0]) != VK_SUCCESS) 
    {
        throw std::runtime_error("failed to record command buffer");
    }

    VkFenceCreateInfo fence_info{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    VkFence compute_fence;
    if (vkCreateFence(device, &fence_info, nullptr, &compute_fence) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create compute fence");
    }

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffers[0];

    if (vkQueueSubmit(compute_queue, 1, &submit_info, compute_fence) != VK_SUCCESS) 
    {
        throw std::runtime_error("failed to submit compute command buffer");
    }

    vkWaitForFences(device, 1, &compute_fence, VK_TRUE, UINT64_MAX);

    void* mapped_memory;
    vkMapMemory(device, buffer_memory, 0, sizeof(float) * 128, 0, &mapped_memory);

    float* float_array = static_cast<float*>(mapped_memory);
    for (int i = 0; i < 128; i++) 
    {
        std::cout << float_array[i] << " "; 
    }

    vkUnmapMemory(device, buffer_memory);

    vkDestroyFence(device, compute_fence, nullptr);

    return 0;
}