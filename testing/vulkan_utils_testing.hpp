#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>
#include <stdexcept>
#include <cstring>
#include <iostream>
#include <optional>
#include <set>
#include <algorithm>
#include <fstream>
#include <cstddef>
#include <format>
#include "glm/glm.hpp"


#ifdef NDEBUG 
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif
 
struct QueueFamilyIndices;

struct SwapChainSupportDetails;

struct Vertex
{
    glm::vec3 position;
    glm::vec3 color;
    glm::vec3 normal;
};

struct UniformBufferObject
{
    alignas(16) glm::mat4 vp;
    alignas(16) glm::vec3 light_dir;
};

struct PushConstantData
{
    alignas(16) glm::mat4 model;
    alignas(16) glm::vec3 color;
};

struct RenderObject
{
    VkBuffer vertex_buffer;
    VkBuffer index_buffer;
    uint32_t index_count;
    PushConstantData push_constants;
    VkDeviceMemory vertex_buffer_memory;
    VkDeviceMemory index_buffer_memory;
};

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);

SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);

VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);

VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window);

bool checkDeviceExtensionSupport(VkPhysicalDevice device);

bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface);

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);

bool check_validation_layer_support(const std::vector<const char*> validation_layers);

VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback();

void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

void setupDebugMessenger(VkInstance instance, VkDebugUtilsMessengerEXT& debug_messenger);

void createSurface(VkInstance instance, GLFWwindow* window, VkSurfaceKHR& surface);

std::vector<const char*> getRequiredExtensions();

std::vector<char> readFile(const std::string& filename);

VkShaderModule createShaderModule(const std::vector<char>& code, VkDevice device);

void create_instance_without_debug(VkInstance& instance, const char* app_name, uint32_t api_version);

void create_instance(VkInstance& instance, const char* app_name, uint32_t api_version);

void pick_physical_device(VkInstance instance, VkPhysicalDevice& physical_device, VkSurfaceKHR surface);

void create_logical_device(VkPhysicalDevice physical_device, VkDevice& device, VkSurfaceKHR surface, VkQueue& graphics_queue, VkQueue& present_queue);

void create_swapchain(VkPhysicalDevice physical_device, VkDevice device, VkSurfaceKHR surface, GLFWwindow* window, VkSwapchainKHR& swap_chain, std::vector<VkImage>& swap_chain_images, VkFormat& swap_chain_image_format, VkExtent2D& swap_chain_extent);

void createImageViews(std::vector<VkImageView>& swap_chain_image_views, std::vector<VkImage> swap_chain_images, VkFormat swap_chain_image_format, VkDevice device);

void create_render_pass(VkFormat swap_chain_color_format, VkDevice device, VkRenderPass& render_pass, VkFormat depth_format, VkSampleCountFlagBits samples);

uint32_t find_memory_type(VkPhysicalDevice physical_device, uint32_t type_filter, VkMemoryPropertyFlags properties);

void create_vertex_buffer(VkDevice device, VkPhysicalDevice physical_device, VkSurfaceKHR surface, std::vector<Vertex> vertices, VkBuffer& vertex_buffer, VkDeviceMemory& vertex_buffer_memory);

void create_uniform_buffer(VkDevice device, VkPhysicalDevice physical_device, std::vector<VkBuffer>& uniform_buffers, std::vector<VkDeviceMemory>& uniform_buffers_memory, std::vector<void*>& uniform_buffers_mapped, int max_frames_in_flight);

void create_descriptor_set_layout(VkDevice device, VkDescriptorSetLayout& descriptor_set_layout);

void create_descriptor_pool(VkDevice device, VkDescriptorPool& descriptor_pool, int max_frames_in_flight);

void create_descriptor_sets(VkDevice device, VkDescriptorSetLayout descriptor_set_layout, VkDescriptorPool descriptor_pool, std::vector<VkDescriptorSet>& descriptor_sets, std::vector<VkBuffer>& uniform_buffers ,int max_frames_in_flight);

void create_graphics_pipeline(const std::string vertex_shader_filepath, const std::string fragment_shader_filepath, VkDevice device, VkPipelineLayout& pipeline_layout, VkRenderPass render_pass, VkPipeline& graphics_pipeline, VkDescriptorSetLayout& descriptor_set_layout);

void create_frame_buffers(std::vector<VkFramebuffer>& swap_chain_frame_buffers, std::vector<VkImageView> swap_chain_image_views, VkRenderPass render_pass, VkExtent2D swap_chain_extent, VkDevice device, const std::vector<VkImageView>& depth_image_views, bool deferred = false, const std::vector<VkImageView>* normal_views = nullptr, const std::vector<VkImageView>* albedo_views = nullptr);

void create_command_pool(VkPhysicalDevice physical_device, VkDevice device, VkSurfaceKHR surface, VkCommandPool& command_pool);

void create_command_buffers(VkCommandPool command_pool, VkDevice device, std::vector<VkCommandBuffer>& command_buffers);

void record_command_buffer(uint32_t image_index, VkCommandBuffer command_buffer, VkRenderPass render_pass, const std::vector<VkFramebuffer>& swap_chain_frame_buffers, VkExtent2D swap_chain_extent, VkPipeline graphics_pipeline, const std::vector<RenderObject>& render_objects, VkDescriptorSet& descriptor_set, VkPipelineLayout pipeline_layout);

void create_sync_objects(VkDevice device, std::vector<VkSemaphore>& image_available_semaphores, std::vector<VkSemaphore>& render_finished_semaphores, std::vector<VkFence>& in_flight_fences, int max_frames_in_flight);

void draw_frame(uint32_t current_frame, VkDevice device, std::vector<VkFence>& in_flight_fences, std::vector<VkCommandBuffer>& command_buffers, VkRenderPass render_pass, const std::vector<VkFramebuffer>& swap_chain_frame_buffers, VkExtent2D swap_chain_extent, VkPipeline graphics_pipeline, const std::vector<RenderObject>& render_objects, VkSwapchainKHR swap_chain, std::vector<VkSemaphore>& image_available_semaphores, std::vector<VkSemaphore>& render_finished_semaphores, VkQueue graphics_queue, VkDescriptorSet& descriptor_set, VkPipelineLayout pipeline_layout, bool deferred=false, VkPipeline* geometry_pipeline=nullptr, VkPipeline* lighting_pipeline=nullptr);

void cleanup(VkDevice device, std::vector<VkSemaphore> render_finished_semaphores, std::vector<VkSemaphore> image_available_semaphores, std::vector<VkFence> in_flight_fences, VkCommandPool command_pool, std::vector<VkFramebuffer> swap_chain_frame_buffers, VkPipeline graphics_pipeline, VkPipelineLayout pipeline_layout, VkRenderPass render_pass, std::vector<VkImageView> swap_chain_image_views, VkSwapchainKHR swap_chain, VkDebugUtilsMessengerEXT debug_messenger, VkSurfaceKHR surface, VkInstance instance, GLFWwindow* window, std::vector<VkBuffer> uniform_buffers, std::vector<VkDeviceMemory> uniform_buffers_memory, VkDescriptorPool descriptor_pool, VkDescriptorSetLayout descriptor_set_layout, int max_frames_in_flight, std::vector<VkImageView> depth_image_views, std::vector<VkImage> depth_images, std::vector<VkDeviceMemory> depth_image_memories, std::vector<RenderObject> render_objects, bool deferred=false, VkPipeline* geometry_pipeline=nullptr, VkPipeline* lighting_pipeline=nullptr, std::vector<VkImage>* normal_images=nullptr, std::vector<VkDeviceMemory>* normal_memories=nullptr, std::vector<VkImageView>* normal_views=nullptr, std::vector<VkImage>* albedo_images=nullptr, std::vector<VkDeviceMemory>* albedo_memories=nullptr, std::vector<VkImageView>* albedo_views=nullptr);

VkFormat find_supported_format(VkPhysicalDevice physical_device, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

VkFormat find_depth_format(VkPhysicalDevice physical_device);

void create_image(VkDevice device, VkPhysicalDevice physical_device, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& image_memory, VkSampleCountFlagBits samples, VkSharingMode sharing_mode);

VkImageView create_image_view(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspect_flags);

template <typename T>
void create_buffer(VkDevice device, VkPhysicalDevice physical_device, const std::vector<T>& buffer_items, VkBuffer& buffer, VkDeviceMemory& buffer_memory, VkBufferUsageFlags buffer_usage, VkSharingMode sharing_mode)
{
    VkBufferCreateInfo buffer_create_info{};
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.size = sizeof(T) * buffer_items.size();
    buffer_create_info.usage = buffer_usage;
    buffer_create_info.sharingMode = sharing_mode;

    if (vkCreateBuffer(device, &buffer_create_info, nullptr, &buffer) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create buffer");
    }

    VkMemoryAllocateInfo memory_allocate_info{};
    memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    
    VkMemoryRequirements memory_requirements{};
    vkGetBufferMemoryRequirements(device, buffer, &memory_requirements);
    uint32_t memory_type_index = find_memory_type(physical_device, memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    memory_allocate_info.memoryTypeIndex = memory_type_index;
    memory_allocate_info.allocationSize = memory_requirements.size;

    if (vkAllocateMemory(device, &memory_allocate_info, nullptr, &buffer_memory) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate memory");
    }

    if (vkBindBufferMemory(device, buffer, buffer_memory, 0) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to bind buffer");
    }

    void* data;

    if (vkMapMemory(device, buffer_memory, 0, sizeof(T) * buffer_items.size(), 0, &data) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to map memory");
    }

    memcpy(data, buffer_items.data(), size_t(buffer_create_info.size));

    vkUnmapMemory(device, buffer_memory);
}

void create_deferred_render_pass(VkFormat swap_chain_color_format, VkSampleCountFlagBits samples, VkFormat depth_format, VkDevice& device, VkRenderPass& render_pass);

void create_deferred_descriptor_set_layout(VkDevice device, VkDescriptorSetLayout& descriptor_set_layout);

void create_deferred_descriptor_pool(VkDevice device, VkDescriptorPool& descriptor_pool, int max_frames_in_flight);

void create_deferred_descriptor_sets(VkDevice device, VkDescriptorSetLayout descriptor_set_layout, VkDescriptorPool descriptor_pool, std::vector<VkDescriptorSet>& descriptor_sets, std::vector<VkBuffer>& uniform_buffers ,int max_frames_in_flight, const std::vector<VkImageView>& depth_views, const std::vector<VkImageView>& normal_views, const std::vector<VkImageView>& albedo_views);

void create_deferred_pipelines(const std::string& geom_vert_filepath, const std::string& geom_frag_filepath, const std::string& light_vert_filepath, const std::string& light_frag_filepath, VkDevice device, VkPipelineLayout& pipeline_layout, VkRenderPass render_pass, VkPipeline& geometry_pipeline, VkPipeline& lighting_pipeline, VkDescriptorSetLayout& descriptor_set_layout);

void record_deferred_command_buffer(uint32_t image_index, VkCommandBuffer command_buffer, VkRenderPass render_pass, const std::vector<VkFramebuffer>& swap_chain_frame_buffers, VkExtent2D swap_chain_extent, VkPipeline geometry_pipeline, VkPipeline lighting_pipeline, const std::vector<RenderObject>& render_objects, VkDescriptorSet& descriptor_set, VkPipelineLayout pipeline_layout);


