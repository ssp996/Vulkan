#version 450 

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragNormal;

layout(set = 0, binding = 0) uniform uniform_buffer_object{
    mat4 vp;
    vec3 light_dir; 
    mat4 filler;
    vec3 filler2;
}ubo;

layout(push_constant) uniform PushConstants{
    mat4 model;
    mat4 light_view_proj;
}pc;

void main()
{
    gl_Position = ubo.vp * pc.model * vec4(inPosition, 1.0);
    fragColor = inColor;
    fragNormal = mat3(pc.model) * inNormal;
}