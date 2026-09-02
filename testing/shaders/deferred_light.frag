#version 450


layout(set = 0, binding = 0) uniform uniform_buffer_object{
    mat4 vp;
    vec3 light_dir;
}ubo;

layout(input_attachment_index = 0, set = 1, binding = 0) uniform subpassInput inputDepth;
layout(input_attachment_index = 1, set = 1, binding = 1) uniform subpassInput inputNormal;
layout(input_attachment_index = 2, set = 1, binding = 2) uniform subpassInput inputAlbedo;

layout(location = 0) out vec4 outColor; // Final output to Swapchain

void main()
{
    vec3 normal = subpassLoad(inputNormal).xyz;
    vec3 albedo = subpassLoad(inputAlbedo).rgb;
    
    if (length(normal) < 0.1) 
    {
        outColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    vec3 lightDir = normalize(ubo.light_dir);
    vec3 light_opposite = -lightDir;

    float diff = max(dot(normal, light_opposite), 0.0);
    float ambient = 0.1;

    vec3 final_light = albedo * (diff + ambient);

    outColor = vec4(final_light, 1.0);
}