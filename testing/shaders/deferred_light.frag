#version 450

layout(set = 0, binding = 0) uniform CameraUBO{
    mat4 view_proj;
    vec3 light_dir;
    mat4 filler;
    vec3 filler2;
}camera;

layout(set = 0, binding = 1) uniform LightUBO{
    mat4 light_view_proj;
    vec3 light_dir;
    mat4 inverse_view_proj;
    vec3 light_color;
}light;

layout(input_attachment_index = 0, set = 1, binding = 0) uniform subpassInput inputDepth;
layout(input_attachment_index = 1, set = 1, binding = 1) uniform subpassInput inputNormal;
layout(input_attachment_index = 2, set = 1, binding = 2) uniform subpassInput inputAlbedo;

layout(set = 2, binding = 0) uniform sampler2DShadow shadow_map;

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outColor; 

vec3 reconstructWorldPos(vec2 uv, float depth) 
{
    vec4 ndc = vec4(uv * 2.0 - 1.0, depth, 1.0);
    
    vec4 worldPos = light.inverse_view_proj * ndc;
    
    return worldPos.xyz / worldPos.w;
}

void main() 
{
    float depth = subpassLoad(inputDepth).r;
    if (depth == 1.0) {
        outColor = vec4(0.0, 0.0, 0.0, 1.0); 
        return;
    }

    vec3 normal = normalize(subpassLoad(inputNormal).xyz);
    vec3 albedo = subpassLoad(inputAlbedo).rgb;

    vec3 fragPos = reconstructWorldPos(inUV, depth);


    vec3 L = normalize(-light.light_dir);
    float NdotL = max(dot(normal, L), 0.0);
    
    vec3 ambient = albedo * 0.05; 
    vec3 diffuse = albedo * light.light_color * NdotL;


    float shadow = 1.0; 
    
    vec4 lightSpace = light.light_view_proj * vec4(fragPos, 1.0);
    vec3 projCoords = lightSpace.xyz / lightSpace.w; 

    if (projCoords.z > 0.0 && projCoords.z < 1.0) 
    {
        projCoords.xy = projCoords.xy * 0.5 + 0.5;
        
        shadow = texture(shadow_map, vec3(projCoords.xy, projCoords.z));
    }

    vec3 finalColor = ambient + (diffuse * shadow);
    outColor = vec4(finalColor, 1.0);
}