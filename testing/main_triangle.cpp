#include <iostream>
#include <stdexcept>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "glm/glm.hpp"

#include "vulkan_utils_testing.hpp"

int main()
{

    constexpr int MAX_FRAMES_IN_FLIGHT  = 2;

    constexpr int width = 800;
    constexpr int height = 800;

    if (!glfwInit())
    {
        throw std::runtime_error("failed to initialize glfw");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    GLFWwindow* window = glfwCreateWindow(width, height, "triangle hopefully", nullptr, nullptr);

    VkInstance instance;
    create_instance(instance, "triangle", VK_API_VERSION_1_4);

    VkDebugUtilsMessengerEXT debug_messenger;
    setupDebugMessenger(instance, debug_messenger);
 
    VkSurfaceKHR surface;
    createSurface(instance, window, surface);

    VkPhysicalDevice physical_device;
    pick_physical_device(instance, physical_device, surface);

    VkDevice device;
    VkQueue graphics_queue;
    VkQueue present_queue;
    create_logical_device(physical_device, device, surface, graphics_queue, present_queue);

    VkSwapchainKHR swap_chain;
    std::vector<VkImage> swap_chain_images;
    VkFormat swap_chain_image_format;
    VkExtent2D swap_chain_extent;
    create_swapchain(physical_device, device, surface, window, swap_chain, swap_chain_images, swap_chain_image_format, swap_chain_extent);

    std::vector<VkImageView> swap_chain_image_views;
    createImageViews(swap_chain_image_views, swap_chain_images, swap_chain_image_format, device);

    VkRenderPass render_pass;
    create_render_pass(swap_chain_image_format, device, render_pass);
    
    VkPipelineLayout pipeline_layout;
    VkPipeline graphics_pipeline;
    create_graphics_pipeline(
        "shaders/test_vertex.spv",
        "shaders/test_fragment.spv",
        device,
        pipeline_layout,
        render_pass, graphics_pipeline
    );

    std::vector<VkFramebuffer> swap_chain_frame_buffers;
    VkFramebuffer frame_buffer;
    create_frame_buffers(
        swap_chain_frame_buffers, 
        swap_chain_image_views,
        render_pass,
        swap_chain_extent,
        device
    );

    VkCommandPool command_pool;
    create_command_pool(physical_device, device, surface, command_pool);

    std::vector<VkCommandBuffer> command_buffers;
    command_buffers.resize(MAX_FRAMES_IN_FLIGHT);
    create_command_buffers(command_pool, device, command_buffers);

    std::vector<VkSemaphore> image_available_semaphores;
    std::vector<VkSemaphore> render_finished_semaphores;
    std::vector<VkFence> in_flight_fences;
    create_sync_objects(device, image_available_semaphores, render_finished_semaphores, in_flight_fences, MAX_FRAMES_IN_FLIGHT);
    const std::vector<Vertex> vertices = {
    // Vertex 1: Top Center (Red)
    Vertex{glm::vec3(0.0f, -0.5f, 0.5f), glm::vec3(1.0f, 0.0f, 0.0f)},
    
    // Vertex 2: Bottom Right (Green)
    Vertex{glm::vec3(0.5f,  0.5f, 0.5f), glm::vec3(0.0f, 1.0f, 0.0f)},
    
    // Vertex 3: Bottom Left (Blue)
    Vertex{glm::vec3(-0.5f, 0.5f, 0.5f), glm::vec3(0.0f, 0.0f, 1.0f)}
    };

    std::vector<VkDeviceSize> vertex_buffer_offsets = {0};

    VkBuffer vertex_buffer;
    VkDeviceMemory vertex_buffer_memory;
    create_vertex_buffer(device, physical_device, surface, vertices, vertex_buffer, vertex_buffer_memory);
    
    std::vector<VkBuffer> vertex_buffers = {vertex_buffer};

    uint32_t current_frame = 0;

    while(!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        draw_frame(
            current_frame,
            device, 
            in_flight_fences,
            command_buffers,
            render_pass,
            swap_chain_frame_buffers,
            swap_chain_extent,
            graphics_pipeline,
            vertex_buffers,
            vertex_buffer_offsets,
            vertices,
            swap_chain,
            image_available_semaphores,
            render_finished_semaphores,
            graphics_queue
        );

        current_frame = (current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
    }
    
    vkDeviceWaitIdle(device);

    cleanup(
        device,
        render_finished_semaphores,
        image_available_semaphores,
        in_flight_fences,
        command_pool,
        swap_chain_frame_buffers,
        graphics_pipeline,
        pipeline_layout,
        render_pass,
        swap_chain_image_views,
        swap_chain,
        debug_messenger,
        surface,
        instance,
        window,
        vertex_buffer_memory,
        vertex_buffer,
        MAX_FRAMES_IN_FLIGHT
    );

    glfwTerminate();

    return 0;
}

