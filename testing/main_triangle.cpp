#include <iostream>
#include <stdexcept>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "vulkan_utils_testing.hpp"

int main()
{

    constexpr int MAX_FRAMES_IN_FLIGHT  = 3;

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

    VkFormat depth_format = find_depth_format(physical_device);
    
    VkRenderPass render_pass;
    create_render_pass(swap_chain_image_format, device, render_pass, depth_format, VK_SAMPLE_COUNT_1_BIT);
    
    VkPipelineLayout pipeline_layout;
    VkPipeline graphics_pipeline;
    VkDescriptorSetLayout descriptor_set_layout;

    create_descriptor_set_layout(device, descriptor_set_layout);

    create_graphics_pipeline(
        "shaders/test_vertex.spv",
        "shaders/test_fragment.spv",
        device,
        pipeline_layout,
        render_pass, graphics_pipeline,
        descriptor_set_layout
    );

    VkImage depth_image;
    VkDeviceMemory depth_image_memory;
    VkImageView depth_image_view;

    create_image(device, physical_device, swap_chain_extent.width, swap_chain_extent.height, depth_format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depth_image, depth_image_memory, VK_SAMPLE_COUNT_1_BIT, VK_SHARING_MODE_EXCLUSIVE);
    depth_image_view = create_image_view(device, depth_image, depth_format, VK_IMAGE_ASPECT_DEPTH_BIT);

    std::vector<VkFramebuffer> swap_chain_frame_buffers;
    VkFramebuffer frame_buffer;
    create_frame_buffers(
        swap_chain_frame_buffers, 
        swap_chain_image_views,
        render_pass,
        swap_chain_extent,
        device,
        depth_image_view
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
    Vertex{glm::vec3(-0.2f, -0.2f, -0.2), glm::vec3(0.0f, 1.0f, 0.0f)},
    
    Vertex{glm::vec3(0.2f,  -0.2f, -0.2f), glm::vec3(0.0f, 1.0f, 0.0f)},
    
    Vertex{glm::vec3(0.2f, 0.2f, -0.2f), glm::vec3(0.0f, 1.0f, 0.0f)},

    Vertex{glm::vec3(-0.2f, 0.2f, -0.2f), glm::vec3(0.0f, 1.0f, 0.0f)},
    
    Vertex{glm::vec3(-0.2f,  -0.2f, 0.2), glm::vec3(0.502, 0.0, 0.502)},

    Vertex{glm::vec3(0.2f, -0.2f, 0.2f), glm::vec3(0.502, 0.0, 0.502)},

    Vertex{glm::vec3(0.2f,  0.2f, 0.2f), glm::vec3(0.502, 0.0, 0.502)},
    
    Vertex{glm::vec3(-0.2f, 0.2f, 0.2f), glm::vec3(0.502, 0.0, 0.502)}

    };

    const std::vector<uint16_t> indices = {
        // Front face
        0, 1, 2, 2, 3, 0,
        // Back face
        4, 7, 6, 6, 5, 4,
        // Left face
        4, 0, 3, 3, 7, 4,
        // Right face
        1, 5, 6, 6, 2, 1,
        // Top face
        3, 2, 6, 6, 7, 3,
        // Bottom face
        4, 5, 1, 1, 0, 4
    };


   std::vector<Vertex> floor_vertices = {
        Vertex{glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec3(0.3f, 0.3f, 0.3f)}, // 0: Top-Left
        Vertex{glm::vec3(1.0f, -1.0f, -1.0f), glm::vec3(0.3f, 0.3f, 0.3f)},  // 1: Top-Right
        Vertex{glm::vec3(1.0f, 1.0f, -1.0f), glm::vec3(0.3f, 0.3f, 0.3f)},   // 2: Bottom-Right (Swapped!)
        Vertex{glm::vec3(-1.0f, 1.0f, -1.0f), glm::vec3(0.3f, 0.3f, 0.3f)}   // 3: Bottom-Left  (Swapped!)
    };

    std::vector<uint16_t> floor_indices = {
        0, 1, 2, 2, 3, 0
    };

    std::vector<VkDeviceSize> vertex_buffer_offsets = {0};

    VkBuffer vertex_buffer;
    VkDeviceMemory vertex_buffer_memory;
    create_vertex_buffer(device, physical_device, surface, vertices, vertex_buffer, vertex_buffer_memory);

    VkBuffer index_buffer;
    VkDeviceMemory index_buffer_memory;
    create_buffer<uint16_t>(device, physical_device, indices, index_buffer, index_buffer_memory, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE);

    std::vector<VkBuffer> uniform_buffers;
    std::vector<VkDeviceMemory> uniform_buffers_memory;
    std::vector<void*> uniform_buffers_mapped;

    create_uniform_buffer(device, physical_device, uniform_buffers, uniform_buffers_memory, uniform_buffers_mapped, MAX_FRAMES_IN_FLIGHT);

    VkDescriptorPool descriptor_pool{};
    create_descriptor_pool(device, descriptor_pool, MAX_FRAMES_IN_FLIGHT);

    std::vector<VkDescriptorSet> descriptor_sets;   
    create_descriptor_sets(device, descriptor_set_layout, descriptor_pool, descriptor_sets, uniform_buffers, MAX_FRAMES_IN_FLIGHT);

    uint32_t current_frame = 0;

    PushConstantData cube_push_constants{};
    cube_push_constants.color = glm::vec3(1.0f, 1.0f, 1.0f);
    cube_push_constants.model = glm::mat4(1.0f);

    RenderObject cube{};
    cube.vertex_buffer = vertex_buffer;
    cube.index_buffer = index_buffer;
    cube.index_count = static_cast<uint32_t>(indices.size());
    cube.push_constants = cube_push_constants;

    VkBuffer floor_buffer;
    VkDeviceMemory floor_memory;
    //create_vertex_buffer and create_buffer call vkAllocateMemory for which there is a hard limit, so in the future switch to Vulkan Memory Allocator or do it yourself
    create_vertex_buffer(device, physical_device, surface, floor_vertices, floor_buffer, floor_memory);

    VkBuffer floor_index_buffer;
    VkDeviceMemory floor_index_memory;
    create_buffer<uint16_t>(device, physical_device, floor_indices, floor_index_buffer, floor_index_memory, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE);


    PushConstantData floor_push_constants{};    
    floor_push_constants.color = glm::vec3(1.0f, 1.0f, 1.0f);
    floor_push_constants.model = glm::mat4(1.0f);


    RenderObject floor{};
    floor.vertex_buffer = floor_buffer;
    floor.index_buffer = floor_index_buffer;
    floor.index_count = static_cast<uint32_t>(floor_indices.size());
    floor.push_constants = floor_push_constants;


    std::vector<RenderObject> render_objects = {cube, floor};
    while(!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        float time = glfwGetTime();
        
        UniformBufferObject ubo{};

        glm::mat4 cube_model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        cube_model = glm::rotate(cube_model, glm::radians(30.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        render_objects[0].push_constants.model = cube_model;

        glm::mat4 floor_model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 2.0f - time/2.0f));
        render_objects[1].push_constants.model = floor_model;


        /* glm::vec3 camera_pos = glm::vec3(0.0f, 5.0f, 0.0f);
        glm::vec3 camera_lookat = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 camera_up = glm::vec3(0.0f, 1.0f, 0.0f);

        glm::vec3 right = glm::normalize(glm::cross(camera_lookat, camera_up)); */

        glm::mat4 view = glm::lookAt(glm::vec3(2.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

        glm::mat4 proj = glm::perspective(glm::radians(45.0f), (float)swap_chain_extent.width / (float)swap_chain_extent.height, 0.1f, 10.0f);
        proj[1][1] *= -1;
        ubo.vp = proj * view;

        memcpy(uniform_buffers_mapped[current_frame], &ubo, sizeof(ubo));

        draw_frame(
            current_frame,
            device, 
            in_flight_fences,
            command_buffers,
            render_pass,
            swap_chain_frame_buffers,
            swap_chain_extent,
            graphics_pipeline,
            render_objects,
            swap_chain,
            image_available_semaphores,
            render_finished_semaphores,
            graphics_queue,
            descriptor_sets[current_frame],
            pipeline_layout
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
        uniform_buffers,
        uniform_buffers_memory,
        descriptor_pool,
        descriptor_set_layout,
        MAX_FRAMES_IN_FLIGHT,
        depth_image_view,
        depth_image,
        depth_image_memory,
        render_objects
    );

    glfwTerminate();

    return 0;
}

