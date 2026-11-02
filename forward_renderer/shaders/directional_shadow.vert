#version 450

layout(location = 0) vec3 inPosition;

layout(push_constant) uniform PushConstants{
    mat4 model;
    mat4 light_space_matrix;
}pc;

void main()
{
    gl_Position = light_space_matrix * model * vec4(inPosition, 1.0);
}