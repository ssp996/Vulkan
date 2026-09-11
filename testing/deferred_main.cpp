#include <iostream>
#include <stdexcept>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "vulkan_utils_testing.hpp"

int main()
{
    try
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
        create_deferred_render_pass(swap_chain_image_format, VK_SAMPLE_COUNT_1_BIT, depth_format, device, render_pass);
        
        VkPipelineLayout pipeline_layout;

        VkPipeline geometry_pipeline;
        VkPipeline lighting_pipeline;


        VkDescriptorSetLayout ubo_descriptor_layout;
        create_custom_descriptor_set_layout(device, ubo_descriptor_layout, {0}, {1}, {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER}, {nullptr}, {VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT});

        VkDescriptorSetLayout g_buffer_descriptor_layout;
        create_custom_descriptor_set_layout(device, g_buffer_descriptor_layout, {0, 1, 2}, {1, 1, 1}, {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT}, {nullptr, nullptr, nullptr}, {VK_SHADER_STAGE_FRAGMENT_BIT, VK_SHADER_STAGE_FRAGMENT_BIT, VK_SHADER_STAGE_FRAGMENT_BIT});

        std::vector<VkDescriptorSetLayout> descriptor_set_layouts = {ubo_descriptor_layout, g_buffer_descriptor_layout};

        create_deferred_pipelines(
            "shaders/deferred_geom_vert.spv",
            "shaders/deferred_geom_frag.spv",
            "shaders/deferred_light_vert.spv",
            "shaders/deferred_light_frag.spv",
            device,
            pipeline_layout,
            render_pass,
            geometry_pipeline,
            lighting_pipeline,
            descriptor_set_layouts
        );

        std::vector<VkImage> depth_images(swap_chain_images.size());
        std::vector<VkDeviceMemory> depth_image_memories(swap_chain_images.size());
        std::vector<VkImageView> depth_image_views(swap_chain_images.size());

        std::vector<VkImage> normal_images(swap_chain_images.size());
        std::vector<VkDeviceMemory> normal_memories(swap_chain_images.size());
        std::vector<VkImageView> normal_views(swap_chain_images.size());
        
        std::vector<VkImage> albedo_images(swap_chain_images.size());
        std::vector<VkDeviceMemory> albedo_memories(swap_chain_images.size());
        std::vector<VkImageView> albedo_views(swap_chain_images.size());

        for (size_t i = 0; i < swap_chain_images.size(); i++)
        {
            create_image(device, physical_device, swap_chain_extent.width, swap_chain_extent.height, depth_format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depth_images[i], depth_image_memories[i], VK_SAMPLE_COUNT_1_BIT, VK_SHARING_MODE_EXCLUSIVE);
            depth_image_views[i] = create_image_view(device, depth_images[i], depth_format, VK_IMAGE_ASPECT_DEPTH_BIT);

            create_image(device, physical_device, swap_chain_extent.width, swap_chain_extent.height, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, normal_images[i], normal_memories[i], VK_SAMPLE_COUNT_1_BIT, VK_SHARING_MODE_EXCLUSIVE);
            normal_views[i] = create_image_view(device, normal_images[i], VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT);

            create_image(device, physical_device, swap_chain_extent.width, swap_chain_extent.height, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, albedo_images[i], albedo_memories[i], VK_SAMPLE_COUNT_1_BIT, VK_SHARING_MODE_EXCLUSIVE);
            albedo_views[i] = create_image_view(device, albedo_images[i], VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
        }
        
        std::vector<VkFramebuffer> swap_chain_frame_buffers;
        VkFramebuffer frame_buffer;
        create_frame_buffers(
            swap_chain_frame_buffers, 
            swap_chain_image_views,
            render_pass,
            swap_chain_extent,
            device,
            depth_image_views,
            true,
            &normal_views,
            &albedo_views
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
            // FRONT FACE (Red) - Normal points towards -Z
            Vertex{glm::vec3(-0.2f, -0.2f, -0.2f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)},
            Vertex{glm::vec3( 0.2f, -0.2f, -0.2f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)},
            Vertex{glm::vec3( 0.2f,  0.2f, -0.2f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)},
            Vertex{glm::vec3(-0.2f,  0.2f, -0.2f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)},

            // BACK FACE (Blue) - Normal points towards +Z
            Vertex{glm::vec3(-0.2f, -0.2f,  0.2f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)},
            Vertex{glm::vec3( 0.2f, -0.2f,  0.2f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)},
            Vertex{glm::vec3( 0.2f,  0.2f,  0.2f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)},
            Vertex{glm::vec3(-0.2f,  0.2f,  0.2f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)},

            // LEFT FACE (Green) - Normal points towards -X
            Vertex{glm::vec3(-0.2f, -0.2f, -0.2f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f)},
            Vertex{glm::vec3(-0.2f, -0.2f,  0.2f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f)},
            Vertex{glm::vec3(-0.2f,  0.2f,  0.2f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f)},
            Vertex{glm::vec3(-0.2f,  0.2f, -0.2f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f)},

            // RIGHT FACE (Yellow) - Normal points towards +X
            Vertex{glm::vec3( 0.2f, -0.2f, -0.2f), glm::vec3(1.0f, 1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f)},
            Vertex{glm::vec3( 0.2f, -0.2f,  0.2f), glm::vec3(1.0f, 1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f)},
            Vertex{glm::vec3( 0.2f,  0.2f,  0.2f), glm::vec3(1.0f, 1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f)},
            Vertex{glm::vec3( 0.2f,  0.2f, -0.2f), glm::vec3(1.0f, 1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f)},

            // TOP FACE (Magenta) - Normal points towards -Y
            Vertex{glm::vec3(-0.2f, -0.2f, -0.2f), glm::vec3(1.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)},
            Vertex{glm::vec3( 0.2f, -0.2f, -0.2f), glm::vec3(1.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)},
            Vertex{glm::vec3( 0.2f, -0.2f,  0.2f), glm::vec3(1.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)},
            Vertex{glm::vec3(-0.2f, -0.2f,  0.2f), glm::vec3(1.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)},

            // BOTTOM FACE (Cyan) - Normal points towards +Y
            Vertex{glm::vec3(-0.2f,  0.2f, -0.2f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)},
            Vertex{glm::vec3( 0.2f,  0.2f, -0.2f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)},
            Vertex{glm::vec3( 0.2f,  0.2f,  0.2f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)},
            Vertex{glm::vec3(-0.2f,  0.2f,  0.2f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)}
        };

        const std::vector<uint16_t> indices = {
            0, 1, 2, 2, 3, 0,       // Front
            4, 5, 6, 6, 7, 4,       // Back
            8, 9, 10, 10, 11, 8,    // Left
            12, 13, 14, 14, 15, 12, // Right
            16, 17, 18, 18, 19, 16, // Top
            20, 21, 22, 22, 23, 20  // Bottom
        };

    std::vector<Vertex> floor_vertices = {
            // 0: Top-Left
            Vertex{glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec3(0.3f, 0.3f, 0.3f), glm::vec3(0.0f, 1.0f, 0.0f)},
            // 1: Top-Right
            Vertex{glm::vec3( 1.0f, -1.0f, -1.0f), glm::vec3(0.3f, 0.3f, 0.3f), glm::vec3(0.0f, 1.0f, 0.0f)},
            // 2: Bottom-Right
            Vertex{glm::vec3( 1.0f, -1.0f,  1.0f), glm::vec3(0.3f, 0.3f, 0.3f), glm::vec3(0.0f, 1.0f, 0.0f)},
            // 3: Bottom-Left
            Vertex{glm::vec3(-1.0f, -1.0f,  1.0f), glm::vec3(0.3f, 0.3f, 0.3f), glm::vec3(0.0f, 1.0f, 0.0f)}
        };

        std::vector<uint16_t> floor_indices = {
            0, 1, 2, 2, 3, 0 // Two counter-clockwise triangles
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
        create_custom_descriptor_pool({VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT}, {MAX_FRAMES_IN_FLIGHT, static_cast<uint32_t>(swap_chain_images.size() * 3)}, static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT + swap_chain_images.size()), descriptor_pool, device);
        
        std::vector<VkDescriptorSet> ubo_descriptor_sets;
        
        const std::vector<uint32_t> ubo_offsets(MAX_FRAMES_IN_FLIGHT, 0);
        const std::vector<uint32_t> ubo_bindings(MAX_FRAMES_IN_FLIGHT, 0);

        create_ubo_descriptor_sets<UniformBufferObject>(MAX_FRAMES_IN_FLIGHT, ubo_descriptor_layout, descriptor_pool, ubo_descriptor_sets, device, uniform_buffers, ubo_offsets, ubo_bindings);

        std::vector<VkDescriptorSet> g_buffer_descriptor_sets;

        create_g_buffer_descriptor_sets(g_buffer_descriptor_layout, static_cast<uint32_t>(swap_chain_images.size()), descriptor_pool, g_buffer_descriptor_sets, device, depth_image_views, normal_views, albedo_views, 0, 1, 2);

        uint32_t current_frame = 0;

        PushConstantData cube_push_constants{};
        cube_push_constants.color = glm::vec3(1.0f, 1.0f, 1.0f);
        cube_push_constants.model = glm::mat4(1.0f);

        RenderObject cube{};
        cube.vertex_buffer = vertex_buffer;
        cube.index_buffer = index_buffer;
        cube.index_count = static_cast<uint32_t>(indices.size());
        cube.push_constants = cube_push_constants;
        cube.vertex_buffer_memory = vertex_buffer_memory;
        cube.index_buffer_memory  = index_buffer_memory;

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
        floor.vertex_buffer_memory = floor_memory;
        floor.index_buffer_memory  = floor_index_memory;


        std::vector<RenderObject> render_objects = {cube, floor};
        while(!glfwWindowShouldClose(window))
        {
            glfwPollEvents();

            float time = glfwGetTime();
            
            UniformBufferObject ubo{};

            glm::mat4 cube_model = glm::rotate(glm::mat4(1.0f), time/5 * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            cube_model = glm::rotate(cube_model, glm::radians(30.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            render_objects[0].push_constants.model = cube_model;

            /* glm::vec3 camera_pos = glm::vec3(0.0f, 5.0f, 0.0f);
            glm::vec3 camera_lookat = glm::vec3(0.0f, 0.0f, 0.0f);
            glm::vec3 camera_up = glm::vec3(0.0f, 1.0f, 0.0f);

            glm::vec3 right = glm::normalize(glm::cross(camera_lookat, camera_up)); */

            glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 1.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

            glm::mat4 proj = glm::perspective(glm::radians(45.0f), (float)swap_chain_extent.width / (float)swap_chain_extent.height, 0.1f, 10.0f);
            proj[1][1] *= -1;
            ubo.vp = proj * view;
            ubo.light_dir = glm::vec3(0.0f, -1.0f, 1.0f);

            memcpy(uniform_buffers_mapped[current_frame], &ubo, sizeof(ubo));


            draw_frame(
                current_frame,
                device, 
                in_flight_fences,
                command_buffers,
                render_pass,
                swap_chain_frame_buffers,
                swap_chain_extent,
                VK_NULL_HANDLE,
                render_objects,
                swap_chain,
                image_available_semaphores,
                render_finished_semaphores,
                graphics_queue,
                ubo_descriptor_sets[current_frame],
                pipeline_layout,
                true,
                &geometry_pipeline,
                &lighting_pipeline,
                &g_buffer_descriptor_sets
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
            VK_NULL_HANDLE,
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
            ubo_descriptor_layout,
            MAX_FRAMES_IN_FLIGHT,
            depth_image_views,
            depth_images,
            depth_image_memories,
            render_objects,
            true,
            &geometry_pipeline,
            &lighting_pipeline,
            &normal_images,
            &normal_memories,
            &normal_views,
            &albedo_images,
            &albedo_memories,
            &albedo_views,
            &g_buffer_descriptor_layout
        );

        glfwTerminate();

        return 0;
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << "\n";
    }; 
}