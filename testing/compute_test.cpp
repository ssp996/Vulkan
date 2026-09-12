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

    return 0;
}