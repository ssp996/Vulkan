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

struct Material{
    AllocatedImage image;
    VkImageView view;
    VkDescriptorSet descriptor_set;
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
    VkDescriptorSet material_descriptor_set;
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

struct Plight{
    alignas(16) glm::vec3 position;
    alignas(16) glm::vec3 color;
    float intensity;
    float far_plane;
};  

struct PointLight{
    glm::vec3 position;
    glm::vec3 color;
    float intensity;
    std::vector<glm::mat4> light_space_matrices;
    float far_plane;

    std::vector<VkImageView> depth_image_views;
    std::vector<VkImage> depth_images;

    Plight to_gpu()
    {
        Plight p;
        p.position = position;
        p.color = color;
        p.intensity = intensity;

        p.far_plane = far_plane;

        return p;
    }

    std::vector<glm::mat4> get_matrices()
    {
        if (light_space_matrices.size() != 6) throw std::runtime_error("wrong number of light space matrices");

        return light_space_matrices;
    }
    
};

struct LightBufferObject{
    uint32_t active_lights; 
    alignas(16) DirLight dir_lights[MAX_DIRECTIONAL_LIGHTS];
};

struct PointLightBufferObject{
    uint32_t active_lights;
    alignas(16) Plight point_lights[MAX_POINT_LIGHTS];
};  

struct CameraUBO{
    alignas(16) glm::vec3 camera_pos;
    alignas(16) glm::mat4 viewproj;
};

struct DirectionalLightPushConstant{
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 light_space_matrix;
};

struct PointLightPushConstant{
    alignas(16) glm::mat4 model;
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

using free_function = std::function<void(std::shared_ptr<void>, VkInstance, VkDevice, VmaAllocator)>;

inline const std::map<ResourceTypes, free_function> free_functions = {
    {
        ResourceTypes::Sempahores,
        [](std::shared_ptr<void>ptr, VkInstance instance, VkDevice device, VmaAllocator allocator)
        {
            auto vec = std::static_pointer_cast<std::vector<VkSemaphore>>(ptr);

            for (auto& semaphore: *vec)
            {
                vkDestroySemaphore(device, semaphore, nullptr);
            }
        }
    },
    {
        ResourceTypes::Fences,
        [](std::shared_ptr<void>ptr, VkInstance instance, VkDevice device, VmaAllocator allocator)
        {
            auto vec = std::static_pointer_cast<std::vector<VkFence>>(ptr);

            for (auto& fence: *vec)
            {
                vkDestroyFence(device, fence, nullptr);
            }
        }
    },
    {
        ResourceTypes::FrameBuffers,
        [](std::shared_ptr<void>ptr, VkInstance instance, VkDevice device, VmaAllocator allocator)
        {
            auto vec = std::static_pointer_cast<std::vector<VkFramebuffer>>(ptr);

            for (auto& frame_buffer: *vec)
            {
                vkDestroyFramebuffer(device, frame_buffer, nullptr);
            }
        }
    },
    {
        ResourceTypes::ImageViews,
        [](std::shared_ptr<void>ptr, VkInstance instance, VkDevice device, VmaAllocator allocator)
        {
            auto vec = std::static_pointer_cast<std::vector<VkImageView>>(ptr);

            for (auto& image_view: *vec)
            {
                vkDestroyImageView(device, image_view, nullptr);
            }
        }
    },
    {
        ResourceTypes::AllocatedImages,
        [](std::shared_ptr<void>ptr, VkInstance instance, VkDevice device, VmaAllocator allocator)
        {
            auto vec = std::static_pointer_cast<std::vector<AllocatedImage>>(ptr);

            for (auto& image: *vec)
            {
                vmaDestroyImage(allocator, image.image, image.allocation);
            }
        }
    },
    {
        ResourceTypes::AllocatedBuffer,
        [](std::shared_ptr<void>ptr, VkInstance instance, VkDevice device, VmaAllocator allocator)
        {
            auto vec = std::static_pointer_cast<std::vector<AllocatedBuffer>>(ptr);

            for (auto& buffer: *vec)
            {
                vmaDestroyBuffer(allocator, buffer.buffer, buffer.allocation);
            }
        }
    },
    {
        ResourceTypes::RenderObjects,
        [](std::shared_ptr<void>ptr, VkInstance instance, VkDevice device, VmaAllocator allocator)
        {
            auto vec = std::static_pointer_cast<std::vector<RenderObject>>(ptr);

            for (auto& object: *vec)
            {
                delete object.mesh;
            }
        }
    },
    {
        ResourceTypes::Samplers,
        [](std::shared_ptr<void>ptr, VkInstance instance, VkDevice device, VmaAllocator allocator)
        {
            auto vec = std::static_pointer_cast<std::vector<VkSampler>>(ptr);

            for (auto& sampler: *vec)
            {
                vkDestroySampler(device, sampler, nullptr);
            }
        }
    },
    {
        ResourceTypes::DescriptorPools,
        [](std::shared_ptr<void>ptr, VkInstance instance, VkDevice device, VmaAllocator allocator)
        {
            auto vec = std::static_pointer_cast<std::vector<VkDescriptorPool>>(ptr);

            for (auto& pool: *vec)
            {
                vkDestroyDescriptorPool(device, pool, nullptr);
            }
        }
    },
    {
        ResourceTypes::Pipelines,
        [](std::shared_ptr<void>ptr, VkInstance instance, VkDevice device, VmaAllocator allocator)
        {
            auto vec = std::static_pointer_cast<std::vector<VkPipeline>>(ptr);

            for (auto& pipeline: *vec)
            {
                vkDestroyPipeline(device, pipeline, nullptr);
            }
        }
    },
    {
        ResourceTypes::PipelineLayouts,
        [](std::shared_ptr<void>ptr, VkInstance instance, VkDevice device, VmaAllocator allocator)
        {
            auto vec = std::static_pointer_cast<std::vector<VkPipelineLayout>>(ptr);

            for (auto& layout: *vec)
            {
                vkDestroyPipelineLayout(device, layout, nullptr);
            }
        }
    },
    {
        ResourceTypes::DescriptorSetLayouts,
        [](std::shared_ptr<void>ptr, VkInstance instance, VkDevice device, VmaAllocator allocator)
        {
            auto vec = std::static_pointer_cast<std::vector<VkDescriptorSetLayout>>(ptr);

            for (auto& set_layout: *vec)
            {
                vkDestroyDescriptorSetLayout(device, set_layout, nullptr);
            }
        }
    },
    {
        ResourceTypes::RenderPasses,
        [](std::shared_ptr<void>ptr, VkInstance instance, VkDevice device, VmaAllocator allocator)
        {
            auto vec = std::static_pointer_cast<std::vector<VkRenderPass>>(ptr);

            for (auto& pass: *vec)
            {
                vkDestroyRenderPass(device, pass, nullptr);
            }
        }
    },
    {
        ResourceTypes::CommandPools,
        [](std::shared_ptr<void>ptr, VkInstance instance, VkDevice device, VmaAllocator allocator)
        {
            auto vec = std::static_pointer_cast<std::vector<VkCommandPool>>(ptr);

            for (auto& command_pool: *vec)
            {
                vkDestroyCommandPool(device, command_pool, nullptr);
            }
        }
    },
    {
        ResourceTypes::SwapChain,
        [](std::shared_ptr<void>ptr, VkInstance instance, VkDevice device, VmaAllocator allocator)
        {
            auto vec = std::static_pointer_cast<std::vector<VkSwapchainKHR>>(ptr);

            for (auto& swapchain: *vec)
            {
                vkDestroySwapchainKHR(device, swapchain, nullptr);
            }
        }
    }
};