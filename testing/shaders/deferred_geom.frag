#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragNormal;

layout(location = 0) out vec4 outNormal;
layout(location = 1) out vec4 outAlbedo;

void main()
{
    outNormal = vec4(normalize(fragNormal), 1.0);

    outAlbedo = vec4(fragColor, 1.0);
}