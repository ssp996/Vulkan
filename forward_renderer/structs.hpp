#pragma once

#include "enums.hpp"
#include "include.hpp"

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
    glm::vec3 intensity;
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
    VkDescriptorType descriptor_type;
    uint32_t descriptor_count;
};