#pragma once

#include "enums.hpp"
#include "include.hpp"
#include "constants.hpp"

struct QueueFamilyIndices{
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct Resources{
    std::set<ResourceTypes> used_types;
    std::map<ResourceTypes, std::shared_ptr<void>> resources;

    template <typename T>
    void push_back(ResourceTypes key, T& item)
    {
        if (resources.find(key) == resources.end())
        {
            resources[key] = std::make_shared<std::vector<T>>();
            used_types.insert(key);
        }

        auto vec = std::static_pointer_cast<std::vector<T>>(resources[key]);
        vec->push_back(item);
    }

    template <typename T>
    std::vector<T>* get_vector(ResourceTypes key)
    {
        auto required = resources.find(key);
        if (required == resources.end())
        {
            return nullptr;
        }

        return std::static_pointer_cast<std::vector<T>>(required->second).get();
    }
};

struct Vertex
{
    glm::vec3 position;
    glm::vec3 color;
    glm::vec3 normal;
    glm::vec2 uv;
};

struct AllocatedImage{
    VkImage image;
    VmaAllocation allocation;
};

struct AllocatedBuffer{
    VkBuffer buffer;
    VmaAllocation allocation;
    size_t padded_size;
    size_t elem_size;
    void* mapped;
};

struct Mesh{
    AllocatedBuffer vertex_buffer;
    AllocatedBuffer index_buffer;
    uint32_t index_count;
};

struct Transform{
    glm::mat4 scale;
    glm::mat4 rotate;
    glm::mat4 translate;

    glm::mat4 model()
    {
        return scale * rotate * translate;
    }
};

struct RenderObject{
    Mesh* mesh;
    glm::mat4 model;
    uint32_t has_texture;
};

struct DirLight{ 
    alignas(16) glm::vec3 direction;
    alignas(16) glm::vec3 color;
    float intensity; 
    alignas(16) glm::mat4 light_space_matrix;
};

struct DirectionalLight{
    glm::vec3 direction;
    glm::vec3 color;
    float intensity;
    glm::mat4 light_space_matrix;

    //for shadow map
    std::vector<VkImageView> depth_image_views;
    std::vector<VkImage> depth_images;

    DirLight to_gpu()
    {
        DirLight d{};
        d.direction = direction;
        d.color = color;
        d.intensity = intensity;
        d.light_space_matrix = light_space_matrix;

        return d;
    }
};


struct DirectionalLightPushConstant{
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 light_space_matrix;
};

struct Mat4IntPc{
    glm::mat4 model;
    uint32_t has_texture;
};

struct Mat4Mat4Pc{
    alignas(16) glm::mat4 m1;
    alignas(16) glm::mat4 m2;
};

struct VertexShaderPc{
    glm::vec4 r0;
    glm::vec4 r1;
    glm::vec4 r2;
    glm::mat4 vp;
};

struct DescriptorInfo{
    VkDescriptorSetLayoutBinding layout_binding;
    VkDescriptorType descriptor_type;
    DescriptorUsage usage_flag;

    std::vector<VkDescriptorBufferInfo> buffer_info;
    std::vector<VkDescriptorImageInfo> image_info;
    std::vector<VkBufferView> texel_buffer_view;
    const void* extension;
};

struct SetLayoutInfo{
    uint32_t set = 0;
    VkDescriptorSetLayout layout = VK_NULL_HANDLE;
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    std::unordered_map<std::string, uint32_t> names; //variable names - bindings

    void print_layout_bindings() const
    {
        for (const auto& [key, value] : names)
        {
            std:: cout << "name: " << key << " | " << "value: " << value << std::endl;
        }
    }

    const VkDescriptorSetLayoutBinding& get_set_layout_binding(const std::string& name)
    const {
        uint32_t index = names.at(name);

        return *std::find_if(bindings.begin(), bindings.end(), [&](auto& b){return b.binding == index;});
    }
};

struct BindingData {
    std::string name;
    std::vector<VkDescriptorBufferInfo> buffers;   
    std::vector<VkDescriptorImageInfo>  images;    
};



struct LightBufferObject{
    alignas(16) glm::vec3 camera_pos;
    uint32_t active_lights; 
    alignas(16) DirLight dir_lights[MAX_DIRECTIONAL_LIGHTS];
};

struct CameraUBO{
    alignas(16) glm::vec3 camera_pos;
    alignas(16) glm::mat4 viewproj;
};

