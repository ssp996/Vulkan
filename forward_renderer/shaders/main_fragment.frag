#version 450

#define MAX_DIR_LIGHTS 4

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inUv;

struct DirLight {
    vec3 direction;
    vec3 color;
    float intensity;
    mat4 lightSpaceMatrix;
};

layout(set = 0, binding = 0) uniform LightUBO{
    vec3 camera_pos;
    uint active_lights;
    DirLight[MAX_DIR_LIGHTS];
}lbo;

layout(set = 0, binding = 1) uniform sampler2DShadow shadow_maps[MAX_DIR_LIGHTS];

layout(push_constant) uniform PC{
    layout(offset = 112) uint has_texture;
}pc;

layout(location = 0) out vec4 outcolor;


vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, sampler2D shadowMap)
{
    vec3 lightDir = normalize(-light.direction);
    float diff = max(dot(normal, lightDir), 0.0);
    
    vec4 fragPosLightSpace = light.lightSpaceMatrix * vec4(fragPos, 1.0);
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    
    float shadow = 0.0;
    if(projCoords.z <= 1.0) 
    {
        float closestDepth = texture(shadowMap, projCoords.xy).r; 
        float currentDepth = projCoords.z;
        float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.001);
        shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;
    }
    
    return light.color * light.intensity * (diff * (1.0 - shadow));
}

void main()
{
    
}