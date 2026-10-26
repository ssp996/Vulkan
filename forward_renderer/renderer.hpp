#pragma once

#include "include.hpp"
#include "structs.hpp"
#include "enums.hpp"
#include "constants.hpp"


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

/* class DescriptorSet
{
    public:
        DescriptorSet(uint32_t s);

    private:
        uint32_t set_number;

        std::vector<DescriptorInfo> descriptor_infos;

        VkDescriptorSetLayout layout;

        std::vector<VkDescriptorSet> descriptor_sets;

        void add_descrpitor_info(DescriptorInfo info);

        void create_set_layout(VkDevice device);
};
 */
class Renderer
{
    private: 
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

        VkSampler shadow_sampler;
        VkDescriptorSet directional_shadow_map_descriptor_set;

        AllocatedBuffer dir_lights_buffer;

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

        VkDescriptorPool descriptor_pool;

        std::vector<VkDescriptorSet> shadow_map_descriptor_sets;


        //spirv reflect
        /* SpvReflectShaderModule module;
        std::vector<SpvReflectDescriptorSet*> reflect_descriptor_sets;
        std::vector<std::vector<VkDescriptorSetLayoutBinding>> all_bindings; 
        std::vector<VkDescriptorSetLayout> descriptor_set_layouts;
        std::vector<std::vector<VkDescriptorSet>> descriptor_sets;*/

        std::vector<SpvReflectShaderModule*>& reflect_shader_modules; 
        std::map<uint32_t, SetLayoutInfo> set_layout_infos;


        glm::mat4 view_matrix;
        glm::mat4 perspective_projection_matrix;

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
        void allocate_shadow_map_images_and_views();
        void allocate_depth_image_views();

        //shader compilation
        std::vector<char> readFile(const std::string& filepath);
        VkShaderModule createShaderModule(const std::vector<char>& code, VkDevice device);

        //buffers
        void Renderer::create_and_map_buffer(size_t size,  uint32_t n_elements, VkBufferUsageFlags buffer_usage);

        //descriptors
        void create_descriptor_pool();
        void allocate_descriptor_set(VkDescriptorSetLayout layout, std::vector<VkDescriptorSet>& descriptor_sets, uint32_t n);
        /* void create_descriptor_sets(std::vector<uint32_t> set_sizes); */
        void create_descriptor_set(VkDescriptorSet& descriptor_set, const SetLayoutInfo& set_layout_info, const std::vector<BindingData>& binding_data);
        VkDescriptorBufferInfo make_buffer_info(const AllocatedBuffer& allocated_buffer, uint32_t frame);
        VkDescriptorImageInfo make_image_info(const VkImageView& image_view, VkSampler sampler, VkImageLayout image_layout);

        void write_set(VkDescriptorSet set, const SetLayoutInfo& set_layout_info, const std::vector<BindingData>& binding_data);

        std::map<uint32_t, SetLayoutInfo> Renderer::build_set_layouts();


        //shadow pass
        void directional_shadow_pass(uint32_t current_frame);
        void create_shadow_pipeline();
        void create_shadow_sampler();
        /* void create_shadow_maps(uint32_t current_frame);
        void create_directional_shadow_map_descriptor_layout(); */
        
        //main pass
        void create_directional_light_descriptors();
        void main_pass(uint32_t current_frame);

        

    public:
        uint32_t width;
        uint32_t height;

        const std::string directional_shadow_vertex_shader_filepath = "shaders/directional_shadow.vert";
};