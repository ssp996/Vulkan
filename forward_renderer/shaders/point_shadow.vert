#version 450
#extension GL_EXT_multiview: enable

layout(location = 0) in vec3 inPosition;

layout(set = 0, binding = 0) uniform shadowUBO{
    mat4 viewproj[6];
}point_light_matrices;

layout(push_constant) uniform PC{
    mat4 model;
}pc;

void main()
{
    gl_Position = point_light_matrices.viewproj[gl_ViewIndex] * pc.model * vec4(inPosition, 1.0);
}