#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragLightDir;

layout(location = 0) out vec4 outColor;

void main()
{
    vec3 norm = normalize(fragNormal);

    vec3 lightDir = normalize(fragLightDir);

    vec3 light_opposite = -lightDir;

    float diff = max(dot(norm, light_opposite), 0.0);
    
    float ambient = 0.1;

    vec3 final_light = fragColor * (diff + ambient);

    outColor = vec4(final_light, 1.0);
}