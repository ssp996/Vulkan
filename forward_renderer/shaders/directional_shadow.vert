#version 450

layout(location = 0) in vec3 inPosition;

layout(push_constant) uniform PushConstants{
    mat4 model;
    mat4 light_space_matrix;
}pc;

void main()
{
    gl_Position = pc.light_space_matrix * pc.model * vec4(inPosition, 1.0);
}