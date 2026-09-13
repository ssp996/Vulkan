#include <iostream>
#include <stdexcept>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "vulkan_utils_testing.hpp"
#include "vulkan_compute.hpp"

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

    VkPipelineLayout compute_pipeline_layout;
    VkPipeline compute_pipeline;




    return 0;
}