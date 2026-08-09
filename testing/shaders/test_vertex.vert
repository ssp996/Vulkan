#version 450 

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

layout(set = 0, binding = 0) uniform uniform_buffer_object{
    mat4 vp;
}ubo;

layout(push_constant) uniform PushConstants{
    mat4 model;
    vec3 color;
}pc;

void main()
{
    gl_Position = ubo.vp * pc.model *  vec4(inPosition, 1.0);
    fragColor = inColor * pc.color;
}