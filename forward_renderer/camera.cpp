#include "camera.hpp"

Camera::Camera(glm::vec3 p, glm::vec3 wu)
{
    position = p;
    world_up = wu;
    point_at(glm::vec3(0.0f));
}

void Camera::point_at(glm::vec3 target)
{
    glm::vec3 d = glm::normalize(target - position);
    pitch = glm::degrees(glm::asin(d.y));
    yaw = glm::degrees(glm::atan(d.z, d.x));
}

glm::vec3 Camera::forward()
{
    float rad_pitch = glm::radians(pitch);
    float rad_yaw = glm::radians(yaw);

    return glm::normalize(
        glm::vec3(
            cosf(rad_pitch) * cosf(rad_yaw), 
            sinf(rad_pitch), 
            cosf(rad_pitch) * sinf(rad_yaw))
    );
}

glm::vec3 Camera::right()
{
    return glm::normalize(
        glm::cross(
            forward(),
            world_up
        )
    );
}

glm::mat4 Camera::get_view_matrix()
{
    return glm::lookAt(
        position,
        position + forward(),
        world_up
    );
}

void Camera::capture(bool capture)
{
    captured = capture;
    have_last_mouse = false;
 
    uint32_t mode = capture ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL;
    glfwSetInputMode(window, GLFW_CURSOR, mode);
}

void Camera::attach_window(GLFWwindow* w)
{
    window = w;
    capture(true);
}

float min(float a, float b)
{  
    return (a < b) ? a : b;
}

bool down(GLFWwindow* window, uint32_t key)
{
    return glfwGetKey(window, key) == GLFW_PRESS;
}

void Camera::update(GLFWwindow* w)
{
    if (window == nullptr)
    {
        attach_window(w);
    }

    float now = glfwGetTime();

    float dt = (last_time < 0) ? 0.0 : min(now - last_time, 0.1f);
    last_time = now;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS && captured)
    {
        capture(false);
    }
    else if (!captured && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
    {
        capture(true);
    }

    if (captured)
    {
        double x, y;
        glfwGetCursorPos(window, &x, &y);

        glm::vec2 curr_mouse_pos = glm::vec2(float(x), float(y));

        if (have_last_mouse)
        {
            glm::vec2 dxdy = curr_mouse_pos - last_mouse;
            yaw += dxdy.x * sens;
            pitch = glm::clamp<float>(pitch - dxdy.y * sens, -89.0f, 89.0f);
        }

        last_mouse = curr_mouse_pos;
        have_last_mouse = true;
    }
    glm::vec3 move = glm::vec3(0.0f);
    if (down(window, GLFW_KEY_W)) move += forward();
    if (down(window, GLFW_KEY_S)) move -= forward();
    if (down(window, GLFW_KEY_D)) move += right();
    if (down(window, GLFW_KEY_A)) move -= right();
    if (down(window, GLFW_KEY_SPACE)) move += world_up;
    if (down(window, GLFW_KEY_LEFT_CONTROL)) move -= world_up;

    if (glm::length(move) > 0.0f)
    {
        position += glm::normalize(move) * speed * dt;
    }
}



