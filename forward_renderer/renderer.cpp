#include "include.hpp"
#include "renderer.hpp"
#include "reflect.hpp"

Renderer::Renderer(){};

glm::mat4 Renderer::get_projection_matrix()
{

    if (swap_chain_extent.height == 0 || swap_chain_extent.width == 0) 
    {
        return glm::mat4(1.0f); 
    }
    float fov    = glm::radians(45.0f);        
    std::cout << "aspect: " << (float)swap_chain_extent.width / (float)swap_chain_extent.height << std::endl;
    float aspect = (float)swap_chain_extent.width / (float)swap_chain_extent.height;            
    float near   = 0.1f;                       
    float far    = 100.0f;     

    glm::mat4 p = glm::perspective(fov, aspect, near, far);
    p[1][1] *= -1;
    return p;
}

glm::mat4 Renderer::get_view_matrix()
{
    // Position the camera high and to the side (X: 4, Y: 3, Z: 2)
    // Looking at the center (0, 0, 0)
    // Up vector is Y-up (0, 1, 0)
    return glm::lookAt(
        glm::vec3(4.0f, 3.0f, 2.0f), 
        glm::vec3(0.0f, 0.0f, 0.0f), 
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
}

RenderObject Renderer::create_render_object(std::vector<glm::vec3>& positions, std::vector<glm::vec3>& colors, std::vector<glm::vec3>& normals, std::vector<glm::vec2>& uvs, std::vector<uint16_t>& indices, uint32_t has_texture)
{   
    Mesh* mesh = new Mesh;
    
    if (positions.size() != colors.size() || positions.size() != normals.size() || positions.size() != uvs.size())
    {
        throw std::runtime_error("vertex attribute count");
    }    

    std::vector<Vertex> vertices(positions.size());

    for (int i = 0; i < positions.size(); i++)
    {
        vertices[i].position = positions[i];
        vertices[i].color = colors[i];
        vertices[i].normal = normals[i];
        vertices[i].uv = uvs[i];
    }


    create_and_map_vertex_buffer(vertices, indices, *mesh);
    mesh->index_count = (uint32_t)indices.size();

    RenderObject r{};

    r.has_texture = has_texture;
    r.mesh = mesh;
    r.model = glm::mat4(1.0f);

    return r;
}

RenderObject Renderer::create_render_object_vertices(std::vector<Vertex>& vertices, std::vector<uint16_t>& indices, uint32_t has_texture)
{   
    Mesh* mesh = new Mesh;

    create_and_map_vertex_buffer(vertices, indices, *mesh);
    mesh->index_count = (uint32_t)indices.size();

    RenderObject r{};

    r.has_texture = has_texture;
    r.mesh = mesh;
    r.model = glm::mat4(1.0f);

    return r;
}

DirectionalLight Renderer::create_directional_light(glm::vec3 direction, glm::vec3 color, float intensity)
{
    float distance = 20.0f;
    float extent = 12.0f;
    float zNear = 0.1f;
    float zFar = 50.0f;

    direction = glm::normalize(direction);
    glm::vec3 eye = -direction * distance;

    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

    if (std::abs(direction.y) > 0.999f) 
    {
        up = glm::vec3(0.0f, 0.0f, 1.0f);
    }

    glm::mat4 view = glm::lookAt(eye, glm::vec3(0.0f), up);

    glm::mat4 proj = glm::ortho(-extent, extent, -extent, extent, zNear, zFar);

    proj[1][1] *= -1.0f;

    glm::mat4 light_space_matrix = proj * view;

    DirectionalLight d{};
    d.direction = direction;
    d.color = color;
    d.intensity = intensity;
    d.light_space_matrix = light_space_matrix;
    d.depth_images.resize(swap_chain_images.size());
    d.depth_image_views.resize(swap_chain_images.size());

    return d;
}

void Renderer::init()
{
    if (!glfwInit())
    {
        throw std::runtime_error("failed to init glfw");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(width, height, "fhafuh", nullptr, nullptr);

    create_instance();
    setupDebugMessenger();
    create_surface();
    pick_physical_device();
    create_logical_device();

    createSwapChain(); //assigns swap_chain_image_format
    swap_chain_image_views.resize(swap_chain_images.size());
    for (int i = 0; i < swap_chain_images.size(); i++)
    {
        swap_chain_image_views[i] = create_swap_chain_image_view(swap_chain_images[i], swap_chain_image_format);
    }

    create_memory_allocator();

    allocate_depth_image_views();

    shader_codes.push_back(readFile(directional_shadow_vertex_shader_filepath));
    shader_codes.push_back(readFile(main_vertex_shader_filepath));
    shader_codes.push_back(readFile(main_fragment_shader_filepath));

    reflect_shader_modules.resize(shader_codes.size());

    for (int i = 0; i < shader_codes.size(); i++)
    {
        load_reflect_module(shader_codes[i], reflect_shader_modules[i]);
    }

    set_layout_infos = build_set_layouts();

    create_shadow_sampler();

    std::cout << "printing names: " << std::endl;
    for (const auto& [key, set_layout_info]: set_layout_infos)
    {
        set_layout_info.print_layout_bindings();
    }

    create_shadow_pipeline();
    create_main_pipeline();

    create_command_pool();
    
    create_command_buffers();

    create_semaphore_set(static_cast<uint32_t>(swap_chain_images.size()), image_available_semaphores);
    create_semaphore_set(static_cast<uint32_t>(swap_chain_images.size()), render_finished_semaphores);
    create_fence_set(static_cast<uint32_t>(swap_chain_images.size()), in_flight_fences, VK_FENCE_CREATE_SIGNALED_BIT);

    create_descriptor_pool();
}

void Renderer::cleanup()
{
    vkDeviceWaitIdle(device);
    
    for (auto it = resources.resources.begin(); it != resources.resources.end(); ++it)
    {
        ResourceTypes type = it->first;
        std::shared_ptr<void> ptr = it->second;

        if (free_functions.find(type) != free_functions.end())
        {
            free_functions.at(type)(ptr, instance, device, allocator);
        }
    }
    
    resources.resources.clear();
    resources.used_types.clear();

    vmaDestroyAllocator(allocator);
    vkDestroyDevice(device, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr); 
    
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debug_messenger, nullptr);
    }
    
    vkDestroyInstance(instance, nullptr);
}

float Renderer::get_time()
{
    return time;
}
