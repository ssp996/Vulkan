#include "renderer.hpp"

int main()
{
    try
    { 
    std::vector<Vertex> cube_vertices = {
        // FRONT FACE (+Z) - Red
        // Position                 Color                 Normal                UV
        {{-0.5f, -0.5f,  0.5f},  {1.0f, 0.0f, 0.0f},  { 0.0f,  0.0f,  1.0f},  {0.0f, 0.0f}}, // Bottom-Left
        {{ 0.5f, -0.5f,  0.5f},  {1.0f, 0.0f, 0.0f},  { 0.0f,  0.0f,  1.0f},  {1.0f, 0.0f}}, // Bottom-Right
        {{ 0.5f,  0.5f,  0.5f},  {1.0f, 0.0f, 0.0f},  { 0.0f,  0.0f,  1.0f},  {1.0f, 1.0f}}, // Top-Right
        {{-0.5f,  0.5f,  0.5f},  {1.0f, 0.0f, 0.0f},  { 0.0f,  0.0f,  1.0f},  {0.0f, 1.0f}}, // Top-Left

        // BACK FACE (-Z) - Green
        {{ 0.5f, -0.5f, -0.5f},  {0.0f, 1.0f, 0.0f},  { 0.0f,  0.0f, -1.0f},  {0.0f, 0.0f}},
        {{-0.5f, -0.5f, -0.5f},  {0.0f, 1.0f, 0.0f},  { 0.0f,  0.0f, -1.0f},  {1.0f, 0.0f}},
        {{-0.5f,  0.5f, -0.5f},  {0.0f, 1.0f, 0.0f},  { 0.0f,  0.0f, -1.0f},  {1.0f, 1.0f}},
        {{ 0.5f,  0.5f, -0.5f},  {0.0f, 1.0f, 0.0f},  { 0.0f,  0.0f, -1.0f},  {0.0f, 1.0f}},

        // LEFT FACE (-X) - Blue
        {{-0.5f, -0.5f, -0.5f},  {0.0f, 0.0f, 1.0f},  {-1.0f,  0.0f,  0.0f},  {0.0f, 0.0f}},
        {{-0.5f, -0.5f,  0.5f},  {0.0f, 0.0f, 1.0f},  {-1.0f,  0.0f,  0.0f},  {1.0f, 0.0f}},
        {{-0.5f,  0.5f,  0.5f},  {0.0f, 0.0f, 1.0f},  {-1.0f,  0.0f,  0.0f},  {1.0f, 1.0f}},
        {{-0.5f,  0.5f, -0.5f},  {0.0f, 0.0f, 1.0f},  {-1.0f,  0.0f,  0.0f},  {0.0f, 1.0f}},

        // RIGHT FACE (+X) - Yellow
        {{ 0.5f, -0.5f,  0.5f},  {1.0f, 1.0f, 0.0f},  { 1.0f,  0.0f,  0.0f},  {0.0f, 0.0f}},
        {{ 0.5f, -0.5f, -0.5f},  {1.0f, 1.0f, 0.0f},  { 1.0f,  0.0f,  0.0f},  {1.0f, 0.0f}},
        {{ 0.5f,  0.5f, -0.5f},  {1.0f, 1.0f, 0.0f},  { 1.0f,  0.0f,  0.0f},  {1.0f, 1.0f}},
        {{ 0.5f,  0.5f,  0.5f},  {1.0f, 1.0f, 0.0f},  { 1.0f,  0.0f,  0.0f},  {0.0f, 1.0f}},

        // TOP FACE (+Y) - Magenta
        {{-0.5f,  0.5f,  0.5f},  {1.0f, 0.0f, 1.0f},  { 0.0f,  1.0f,  0.0f},  {0.0f, 0.0f}},
        {{ 0.5f,  0.5f,  0.5f},  {1.0f, 0.0f, 1.0f},  { 0.0f,  1.0f,  0.0f},  {1.0f, 0.0f}},
        {{ 0.5f,  0.5f, -0.5f},  {1.0f, 0.0f, 1.0f},  { 0.0f,  1.0f,  0.0f},  {1.0f, 1.0f}},
        {{-0.5f,  0.5f, -0.5f},  {1.0f, 0.0f, 1.0f},  { 0.0f,  1.0f,  0.0f},  {0.0f, 1.0f}},

        // BOTTOM FACE (-Y) - Cyan
        {{-0.5f, -0.5f, -0.5f},  {0.0f, 1.0f, 1.0f},  { 0.0f, -1.0f,  0.0f},  {0.0f, 0.0f}},
        {{ 0.5f, -0.5f, -0.5f},  {0.0f, 1.0f, 1.0f},  { 0.0f, -1.0f,  0.0f},  {1.0f, 0.0f}},
        {{ 0.5f, -0.5f,  0.5f},  {0.0f, 1.0f, 1.0f},  { 0.0f, -1.0f,  0.0f},  {1.0f, 1.0f}},
        {{-0.5f, -0.5f,  0.5f},  {0.0f, 1.0f, 1.0f},  { 0.0f, -1.0f,  0.0f},  {0.0f, 1.0f}}
    };

    // 36 Indices (6 per face, forming 2 triangles each)
    // Defined in Counter-Clockwise (CCW) winding order
    std::vector<uint16_t> cube_indices = {
        0,  1,  2,      2,  3,  0,  // Front
        4,  5,  6,      6,  7,  4,  // Back
        8,  9,  10,     10, 11, 8,  // Left
        12, 13, 14,     14, 15, 12, // Right
        16, 17, 18,     18, 19, 16, // Top
        20, 21, 22,     22, 23, 20  // Bottom
    };

    std::vector<Vertex> floor_vertices = {
        // Position                      Color                 Normal                UV
        {{-10.0f, -0.5f,  10.0f},  {0.8f, 0.8f, 0.8f},  {0.0f, 1.0f, 0.0f},  {0.0f, 0.0f}}, // Bottom-Left
        {{ 10.0f, -0.5f,  10.0f},  {0.8f, 0.8f, 0.8f},  {0.0f, 1.0f, 0.0f},  {1.0f, 0.0f}}, // Bottom-Right
        {{ 10.0f, -0.5f, -10.0f},  {0.8f, 0.8f, 0.8f},  {0.0f, 1.0f, 0.0f},  {1.0f, 1.0f}}, // Top-Right
        {{-10.0f, -0.5f, -10.0f},  {0.8f, 0.8f, 0.8f},  {0.0f, 1.0f, 0.0f},  {0.0f, 1.0f}}  // Top-Left
    };

    std::vector<uint16_t> floor_indices = {
        0,  1,  2,      2,  3,  0
    };

    Renderer renderer;

    renderer.init();

    DirectionalLight l1 = renderer.create_directional_light(glm::vec3(0.0f, -1.0f, -1.0f), glm::vec3(0.6f, 0.2f, 0.2f), 0.4);

    RenderObject cube = renderer.create_render_object_vertices(cube_vertices, cube_indices, 0);
    RenderObject floor = renderer.create_render_object_vertices(floor_vertices, floor_indices, 0);

    renderer.directional_lights.push_back(l1);
    renderer.render_objects.push_back(cube);
    renderer.render_objects.push_back(floor);

    renderer.create_directional_light_descriptors();

    uint32_t current_frame = 0;

    renderer.perspective_projection_matrix = renderer.get_projection_matrix();
    renderer.camera_pos = glm::vec3(4.0f, 3.0f, 2.0f);

    while (!glfwWindowShouldClose(renderer.window))
    {
        glfwPollEvents();

        renderer.draw_frame(current_frame);

        current_frame = (current_frame + 1) % MAX_FRAMES_IN_FLIGHT; 
    }

    vkDeviceWaitIdle(renderer.device);

    //add cleanup next

    glfwTerminate();

    return 0;

    }
    catch(const std::exception& e)
    {
        std::cout << "exception: " <<  e.what() << std::endl;
    } 
        
}