#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inUv;

layout(location = 0) out vec3 outPosition;
layout(location = 1) out vec3 outColor;
layout(location = 2) out vec3 outNormal;
layout(location = 3) out vec2 outUv;

layout(push_constant) uniform PC{
    vec4 model_r0;
    vec4 model_r1;
    vec4 model_r2;
    mat4 viewproj;
}pc;


void main()
{
    vec4 p = vec4(inPosition, 1.0);
    vec3 world = vec3(dot(pc.model_r0, p), dot(pc.model_r1, p), dot(pc.model_r2, p));

    gl_Position = pc.viewproj * vec4(world, 1.0);

    outPosition = world;
    outColor = inColor;
     outNormal   = vec3(dot(pc.model_r0.xyz, inNormal),
                       dot(pc.model_r1.xyz, inNormal),
                       dot(pc.model_r2.xyz, inNormal));
    outUv = inUv;
}