#version 450

#define MAX_DIR_LIGHTS 4
#define MAX_POINT_LIGHTS 4

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

struct PointLight{
    vec3 position;
    vec3 color;
    float intensity;
    float far_plane;
};

layout(set = 0, binding = 0) uniform LightUBO{
    uint active_lights;
    DirLight lights[MAX_DIR_LIGHTS];
}lbo;

layout(set = 0, binding = 1) uniform PointLightUBO{
    uint active_lights;
    PointLight lights[MAX_POINT_LIGHTS];
}pbo;

layout(set = 0, binding = 2) uniform sampler2DShadow shadow_maps[MAX_DIR_LIGHTS];

layout(set = 0, binding = 3) uniform samplerCubeShadow point_light_shadow_maps[MAX_POINT_LIGHTS];

layout(set = 0, binding = 4) uniform CameraUBO{
    vec3 camera_pos;
    mat4 viewproj;
}cbo;

layout(set = 1, binding = 0) uniform sampler2D albedo_texture;

layout(push_constant) uniform PC{
    mat4 model;
    uint has_texture;
}pc;

layout(location = 0) out vec4 outcolor;


vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, sampler2DShadow shadowMap)
{
    vec3 lightDir = normalize(-light.direction);
    float diff = max(dot(normal, lightDir), 0.0);
    
    vec4 fragPosLightSpace = light.lightSpaceMatrix * vec4(inPosition, 1.0);
    
    // Perspective divide (required if you ever use point lights, good practice here)
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    
    // VULKAN FIX: Only transform XY! 
    // Vulkan's clip space Z is already [0.0 to 1.0]. 
    projCoords.xy = projCoords.xy * 0.5 + 0.5;
    
    // Default to fully lit
    float visibility = 1.0;
    
    // Only apply shadows if the fragment is within the light's frustum
    if(projCoords.z >= 0.0 && projCoords.z <= 1.0) 
    {
        // Apply bias directly to the reference depth to prevent shadow acne
        float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.001);
        float referenceDepth = projCoords.z - bias;
        
        vec2 texelSize = 1.0/textureSize(shadowMap, 0);
        float totalShadow = 0.0;

        for (int x = -1; x <= 1; x++)
        {
            for (int y = -1; y <= 1; y++)
            {
                vec2 offset = vec2(x, y) * texelSize;

                totalShadow += texture(shadowMap, vec3(projCoords.xy + offset, referenceDepth));
            }
        }

        visibility = totalShadow/9.0;
    }

    // Because visibility is already 1.0 (lit) or 0.0 (shadowed), 
    // we no longer need the (1.0 - shadow) inversion.
    return light.color * light.intensity * (diff * visibility);
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 viewDir, samplerCubeShadow shadowMap)
{
    vec3 toLight = light.position - inPosition;
    float dist   = length(toLight);

    // Outside the light's range: no contribution
    if (dist >= light.far_plane) return vec3(0.0);

    vec3 lightDir = toLight / dist;
    float diff = max(dot(normal, lightDir), 0.0);

    // Direction from the light to the fragment is the cube lookup vector
    vec3 fragToLight = -toLight;

    // Depth stored in the map is projected depth along the dominant axis,
    // not Euclidean distance
    vec3 a = abs(fragToLight);
    float d = max(a.x, max(a.y, a.z));

    float n = 0.1;
    float f = light.far_plane;
    float refDepth = f * (d - n) / (d * (f - n));   // matches perspective ZO depth

    // Slope-scaled bias: larger at grazing angles
    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.001);
    refDepth -= bias;

    // PCF: offset the lookup vector by a few taps, scaled by texel size
    // world-space size of one texel at this distance
    float texelWorld = 2.0 * d / float(textureSize(shadowMap, 0).x);

    float visibility = 0.0;
    for (int x = -1; x <= 1; x++)
    {
        for (int y = -1; y <= 1; y++)
        {
            // build two vectors perpendicular to the lookup direction
            vec3 dir = normalize(fragToLight);
            vec3 up  = abs(dir.y) < 0.99 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
            vec3 t   = normalize(cross(dir, up));
            vec3 b   = cross(dir, t);

            vec3 sampleDir = fragToLight + (t * float(x) + b * float(y)) * texelWorld;
            visibility += texture(shadowMap, vec4(sampleDir, refDepth));
        }
    }
    visibility /= 9.0;

    // Smooth distance attenuation that reaches zero at far_plane
    float ratio = dist / f;
    float atten = clamp(1.0 - ratio * ratio * ratio * ratio, 0.0, 1.0);
    atten = atten * atten / (1.0 + dist * dist);

    return light.color * light.intensity * (diff * visibility * atten);
}

void main()
{
    vec3 norm = normalize(inNormal);
    vec3 viewDir = normalize(cbo.camera_pos - inPosition);

    vec3 result = vec3(0.1f);

    for (uint i = 0; i < lbo.active_lights; i++)
    {
        result += CalcDirLight(lbo.lights[i], norm, viewDir, shadow_maps[i]);
    }

    for (uint i = 0; i < pbo.active_lights; i++)
    {
        result += CalcPointLight(pbo.lights[i], norm, viewDir, point_light_shadow_maps[i]);
    }

    vec3 albedo;

    if (pc.has_texture != 0)
    {
        albedo = texture(albedo_texture, inUv).rgb;
    }
    else 
    {
        albedo = inColor;
    }

    outcolor = vec4(result * albedo, 1.0);
}