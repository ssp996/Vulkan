#pragma once
#include "include.hpp"

class Camera{
    private:
        
        void point_at(glm::vec3 target);

        glm::vec3 forward();

        glm::vec3 right();

        void attach_window(GLFWwindow* w);

        void capture(bool capture);

    public:
        Camera(glm::vec3 p=glm::vec3(0.0f, 4.5f, 9.0f), glm::vec3 wu=glm::vec3(0.0f, 1.0f, 0.0f));

        glm::vec3 position;
        glm::vec3 world_up;

        float pitch = 0.0f;
        float yaw = -90.0f;

        float sens = 0.1f;
        float speed = 5.0f;
        float sprint_multiplier = 2.5f;

        //float fov; will be used later to integrate fov kick n shi

        GLFWwindow* window = nullptr;
        glm::vec2 last_mouse = glm::vec2(-1.0f);
        float last_time = -1.0;

        bool captured = false;

        bool have_last_mouse;

        glm::mat4 get_view_matrix();

        void update(GLFWwindow* w);
};