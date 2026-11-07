#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inUv;

layout(location = 0) out vec3 outPosition;
layout(location = 1) out vec3 outColor;
layout(location = 2) out vec3 outNormal;
layout(location = 3) out vec2 outUv;

layout(set = 0, binding = 2) uniform CameraUBO{
    vec3 camera_pos;
    mat4 viewproj;
}cbo;

layout(push_constant) uniform PC{
    mat4 model;
    uint has_texture;
}pc;

void main()
{
    vec4 p = vec4(inPosition, 1.0);
    vec3 world = vec3(pc.model * p);
    
    gl_Position = cbo.viewproj * vec4(world, 1.0);

    outPosition = world;
    outColor = inColor;
    outNormal   = mat3(transpose(inverse(pc.model))) * inNormal;
    outUv = inUv;
}