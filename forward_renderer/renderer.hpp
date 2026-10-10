#pragma once

#include "include.hpp"
#include "structs.hpp"
#include "enums.hpp"


#ifdef NDEBUG 
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif
 
const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
}; 

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

void create_custom_descriptor_set_layout(VkDevice device, VkDescriptorSetLayout& descriptor_set_layout, const std::vector<uint32_t>& bindings, const std::vector<uint32_t>& descriptor_counts, const std::vector<VkDescriptorType>& descriptor_types, const std::vector<VkSampler*> samplers, const std::vector<VkShaderStageFlags>& stage_flags);


class Renderer
{
    private: 
        uint32_t MAX_FRAMES_IN_FLIGHT = 3;

        GLFWwindow* window;

        VkInstance instance;
        VkDebugUtilsMessengerEXT debug_messenger;
        VkSurfaceKHR surface;

        VkPhysicalDevice physical_device = VK_NULL_HANDLE;
        VkDevice device;

        VmaAllocator allocator;

        VkQueue graphics_queue;
        VkQueue present_queue;

        VkSwapchainKHR swap_chain;
        std::vector<VkImage> swap_chain_images;
        VkFormat swap_chain_image_format;
        VkExtent2D swap_chain_extent;
        std::vector<VkImageView> swap_chain_image_views;
        std::vector<VkFramebuffer> swap_chain_frame_buffers;

        std::vector<VkImageView> depth_image_views;
        std::vector<VkImage> depth_images;

        VkFormat shadow_map_format = VK_FORMAT_D32_SFLOAT;
        VkExtent2D directional_shadow_map_extent = {2048, 2048};
        VkPipelineLayout directional_shadow_pipeline_layout;
        VkPipeline directional_shadow_pipeline;

        VkPipelineLayout main_pipeline_layout;
        VkPipeline main_pipeline;

        VkCommandPool command_pool;
        std::vector<VkCommandBuffer> main_command_buffers;

        std::vector<VkSemaphore> shadow_pass_finished_semaphores;
        std::vector<VkSemaphore> image_available_semaphores;
        std::vector<VkSemaphore> render_finished_semaphores;

        std::vector<VkFence> in_flight_fences;

        Resources resources{};

        std::vector<RenderObject> render_objects;
        std::vector<DirectionalLight> directional_lights;

        uint32_t MAX_DESCRIPTOR_SETS = 1000;

        VkDescriptorPool descriptor_pool;

        VkDescriptorSetLayout ubo_descriptor_layout;
        VkDescriptorSetLayout sampler_descriptor_layout;

        std::vector<VkDescriptorSet> ubo_descriptor_sets;

        //debug functions
        void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
        void setupDebugMessenger();
        std::vector<const char*> getRequiredExtensions();
        bool checkValidationLayerSupport();
        static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
        bool checkDeviceExtensionSupport(VkPhysicalDevice device);
        SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
        bool isDeviceSuitable(VkPhysicalDevice device);
        
        //instance creation and other device initialisation
        void create_instance();
        void create_surface();
        void pick_physical_device();
        void create_memory_allocator();
        void create_logical_device();
        
        //swap chain creation 
        void createSwapChain();

        //image views and images
        VkImageView create_swap_chain_image_view(VkImage image, VkFormat format);
        VkImageView create_depth_image_view(VkImage image, VkFormat format);
        void create_image(VkImage& image, VkFormat format, VkImageUsageFlags usage);
        void allocate_depth_images_and_views();

        //shader compilation
        std::vector<char> readFile(const std::string& filepath);
        VkShaderModule createShaderModule(const std::vector<char>& code, VkDevice device);

        //descriptors
        void create_descriptor_pool();

        //shadow pass
        void directional_shadow_pass(uint32_t current_frame);
        void create_shadow_pipeline();

        //main pass
        void main_pass(uint32_t current_frame);

        

    public:
        uint32_t width;
        uint32_t height;

        const std::string directional_shadow_vertex_shader_filepath = "shaders/directional_shadow.vert";
};