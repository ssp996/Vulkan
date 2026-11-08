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
        VkInstance instance;
        VkDebugUtilsMessengerEXT debug_messenger;
        VkSurfaceKHR surface;

        VkPhysicalDevice physical_device = VK_NULL_HANDLE;
        VmaAllocator allocator;

        VkQueue graphics_queue;
        VkQueue present_queue;

        VkSwapchainKHR swap_chain;
        std::vector<VkImage> swap_chain_images;
        VkFormat swap_chain_image_format;
        VkExtent2D swap_chain_extent;
        std::vector<VkImageView> swap_chain_image_views;
        std::vector<VkFramebuffer> swap_chain_frame_buffers;

        VkFormat depth_format;
        std::vector<VkImageView> depth_image_views;
        std::vector<VkImage> depth_images;

        std::vector<VkImage> fallback_image; //allocate with format VK_FORMAT_D32_SFLOAT clear to 1.0f
        std::vector<VkImageView> fallback_image_view;
        VkExtent2D fallback_extent = {1, 1};

        VkFormat shadow_map_format = VK_FORMAT_D32_SFLOAT;
        VkExtent2D directional_shadow_map_extent = {2048, 2048};
        VkPipelineLayout directional_shadow_pipeline_layout;
        VkPipeline directional_shadow_pipeline;

        VkSampler shadow_sampler;
        VkDescriptorSet directional_shadow_map_descriptor_set;

        AllocatedBuffer dir_lights_buffer;
        AllocatedBuffer camera_ubo_buffer;

        VkPipelineLayout main_pipeline_layout;
        VkPipeline main_pipeline;

        VkCommandPool command_pool;
        std::vector<VkCommandBuffer> main_command_buffers;

        VkDescriptorPool descriptor_pool;

        std::vector<VkDescriptorSet> main_pass_descriptor_sets;
        std::vector<VkDescriptorSet> vertex_shader_descriptor_set;


        //spirv reflect
        /* SpvReflectShaderModule module;
        std::vector<SpvReflectDescriptorSet*> reflect_descriptor_sets;
        std::vector<std::vector<VkDescriptorSetLayoutBinding>> all_bindings; 
        std::vector<VkDescriptorSetLayout> descriptor_set_layouts;
        std::vector<std::vector<VkDescriptorSet>> descriptor_sets;*/

        std::vector<std::vector<char>> shader_codes;

        std::vector<SpvReflectShaderModule> reflect_shader_modules; 
        std::map<uint32_t, SetLayoutInfo> set_layout_infos;

        std::vector<VkFence> in_flight_fences;
        std::vector<VkSemaphore> image_available_semaphores;
        std::vector<VkSemaphore> render_finished_semaphores;
        
        std::vector<LightBufferObject> lbos;

        float time;

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
        void create_image(VkImage& image, VkFormat format, VkImageUsageFlags usage, VkExtent2D extent);
        void allocate_shadow_map_images_and_views();
        void allocate_depth_image_views();

        //shader compilation
        std::vector<char> readFile(const std::string& filepath);
        VkShaderModule createShaderModule(const std::vector<char>& code, VkDevice device);

        //buffers
        template <typename T>
        void create_staging_buffer(const std::vector<T>& data, VkBufferUsageFlagBits buffer_usage, AllocatedBuffer& buffer)
        {
            size_t size = sizeof(T) * data.size();

            VkBuffer staging_buffer;
            VmaAllocation staging_allocation;
            
            VkBufferCreateInfo staging_info{};
            staging_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            staging_info.size = (VkDeviceSize)size;
            staging_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT; 
            
            VmaAllocationCreateInfo staging_alloc_info{};
            staging_alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
            staging_alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
            
            VmaAllocationInfo alloc_info;
            vmaCreateBuffer(allocator, &staging_info, &staging_alloc_info, &staging_buffer, &staging_allocation, &alloc_info);
        
            memcpy(alloc_info.pMappedData, data.data(), size);

            VkBufferCreateInfo gpu_info{};
            gpu_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            gpu_info.size = (VkDeviceSize)size;
            gpu_info.usage = buffer_usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT; 
            
            VmaAllocationCreateInfo gpu_alloc_info{};
            gpu_alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE; 
            
            vmaCreateBuffer(allocator, &gpu_info, &gpu_alloc_info, &buffer.buffer, &buffer.allocation, nullptr);
            copy_buffer(staging_buffer, buffer.buffer, size);
            vmaDestroyBuffer(allocator, staging_buffer, staging_allocation);

            resources.push_back(ResourceTypes::AllocatedBuffer, buffer);
        }

        void copy_buffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size);
        void create_and_map_descriptor_buffer(size_t size,  uint32_t n_elements, VkBufferUsageFlags buffer_usage, AllocatedBuffer& buffer);
        void create_and_map_vertex_buffer(const std::vector<Vertex>& vertices, std::vector<uint16_t> indices, Mesh& mesh);
        void create_staging_vertex_buffer(const std::vector<Vertex>& vertices, const std::vector<uint16_t>& indices, Mesh& mesh);
        void create_directional_light_buffer_objects(uint32_t n);
        //descriptors
        void create_descriptor_pool();
        void allocate_descriptor_set(VkDescriptorSetLayout layout, std::vector<VkDescriptorSet>& descriptor_sets, uint32_t n);
        /* void create_descriptor_sets(std::vector<uint32_t> set_sizes); */
        void create_descriptor_set(VkDescriptorSet& descriptor_set, const SetLayoutInfo& set_layout_info, const std::vector<BindingData>& binding_data);
        VkDescriptorBufferInfo make_buffer_info(const AllocatedBuffer& allocated_buffer, uint32_t frame);
        VkDescriptorImageInfo make_image_info(const VkImageView& image_view, VkSampler sampler, VkImageLayout image_layout);

        void write_set(VkDescriptorSet set, const SetLayoutInfo& set_layout_info, const std::vector<BindingData>& binding_data);

        std::map<uint32_t, SetLayoutInfo> build_set_layouts();


        //shadow pass
        void directional_shadow_pass(uint32_t current_frame);
        void create_shadow_pipeline();
        void create_shadow_sampler();
        /* void create_shadow_maps(uint32_t current_frame);
        void create_directional_shadow_map_descriptor_layout(); */
        
        //main pass
        void fallback_fix();
        void create_vertex_shader_descriptor();
        void main_pass(uint32_t current_frame, uint32_t image_index);
        void create_main_pipeline();
        void update_dir_lights_ubo(uint32_t current_frame);
        void update_camera_ubo(uint32_t current_frame);

        //sync
        void create_command_pool();
        void create_command_buffers();
        void transition_image(VkCommandBuffer cmd, VkImage image, VkImageAspectFlags aspect, VkImageLayout old_layout, VkImageLayout new_layout, VkPipelineStageFlags2 src_stage, VkAccessFlags2 src_access, VkPipelineStageFlags2 dst_stage, VkAccessFlags2 dst_access);
        void create_semaphore_set(uint32_t n, std::vector<VkSemaphore>& semaphores);
        void create_fence_set(uint32_t n, std::vector<VkFence>& fences, VkFenceCreateFlagBits create_flags);

    public:
        Renderer();

        const uint32_t width = 800;
        const uint32_t height = 800;

        const std::string directional_shadow_vertex_shader_filepath = "shaders/directional_shadow.spv";

        const std::string main_vertex_shader_filepath = "shaders/main_vertex.spv";
        const std::string main_fragment_shader_filepath = "shaders/main_fragment.spv";

        glm::mat4 get_projection_matrix();
        glm::mat4 get_view_matrix();

        glm::mat4 view_matrix = get_view_matrix();
 
        glm::mat4 perspective_projection_matrix = get_projection_matrix();
        glm::vec3 camera_pos = glm::vec3(0.0f, 0.0f, 0.0f);

        std::vector<RenderObject> render_objects;
        std::vector<DirectionalLight> directional_lights;

        GLFWwindow* window;

        void init();
        
        float get_time();

        void draw_frame(uint32_t current_frame);

        DirectionalLight create_directional_light(glm::vec3 direction, glm::vec3 color, float intensity);
        RenderObject create_render_object(std::vector<glm::vec3>& positions, std::vector<glm::vec3>& colors, std::vector<glm::vec3>& normals, std::vector<glm::vec2>& uvs, std::vector<uint16_t>& indices, uint32_t has_texture);
        RenderObject create_render_object_vertices(std::vector<Vertex>& vertices, std::vector<uint16_t>& indices, uint32_t has_texture);

        void create_main_pass_descriptors(); //temporarily here

        void cleanup();

        //bruh
        VkDevice device;

        Resources resources{};
};  