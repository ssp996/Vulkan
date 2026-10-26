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
    std::unordered_map<ResourceTypes, std::any> resources;

    template <typename T>
    void push_back(ResourceTypes key, T& item)
    {
        if (resources.find(key) == resources.end())
        {
            resources[key] = std::vector<T>();
            used_types.insert(key);
        }

        auto* vec = std::any_cast<std::vector<T>>(&resources[key]);
        if (vec)
        {
            vec->push_back(item);
        }
        else 
        {
            throw std::runtime_error(std::format("type mismatch while inserting {}",static_cast<uint32_t>(key)));
        }
    }

    template <typename T>
    std::vector<T>* get_vector(ResourceTypes key)
    {
        auto required = resources.find(key);
        if (required == resources.end())
        {
            return nullptr;
        }

        return std::any_cast<std::vector<T>>(&(required->second));
    }
};

struct Vertex
{
    glm::vec3 position;
    glm::vec3 color;
    glm::vec3 normal;
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
};

struct DirectionalLight{
    glm::vec3 direction;
    glm::vec3 color;
    float intensity;
    glm::mat4 light_space_matrix;

    //for shadow map
    std::vector<VkImageView> depth_image_views;
    std::vector<VkImage> depth_images;
};


struct DirectionalLightPushConstant{
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 light_space_matrix;
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

struct DirLight{ 
    alignas(16) glm::vec3 direction;
    alignas(16) glm::vec3 color;
    alignas(16) float intensity;
    alignas(16) glm::mat4 light_space_matrix;
};

struct LightBufferObject{
    alignas(16) uint32_t active_lights;
    alignas(16)  DirLight dir_lights[MAX_DIRECTIONAL_LIGHTS];
};

