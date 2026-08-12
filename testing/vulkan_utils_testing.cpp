#include "vulkan_utils_testing.hpp"

//validation layers
const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
}; 

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

//struct to hold indices of graphics queue and present queue families (presentation isn't really a queue operation but more like OS related)
struct QueueFamilyIndices 
{
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() 
    {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }

    uint32_t size()
    {
        return (static_cast<uint32_t>(graphicsFamily.has_value()) + static_cast<uint32_t>(presentFamily.has_value()));
    }
};

//capabilities of the surface, ie what my display can do such as resolution + refresh rate (VkSurfaceCapabilitiesKHR), pixel color format, is it hdr or sdr (vector of VkSurfaceFormatKHR),
//synchronisation settings (vsync on or off or triple buffering on or off (vector of VkPresentModeKHR))
struct SwapChainSupportDetails 
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) 
{
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    //get queue family count (usually like 3)
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    //vector of queue families
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    //populate that vector of queue families
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto& queueFamily : queueFamilies) 
    {   //check if each family has the graphics queue bit flag
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) 
        {
            indices.graphicsFamily = i;
        }

        VkBool32 presentSupport = false;
        //check for surface support (present queue)
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

        if (presentSupport) 
        {
            indices.presentFamily = i;
        }

        //if both are found stop searching
        if (indices.isComplete())
        {
            break;
        }

        i++;
    }

    return indices;
}

SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) 
{
    SwapChainSupportDetails details;

    //populate the capabilities field of the SwapChainSupportDetails (populating the VkSurfaceCapabilitiesKHR struct)
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    
    uint32_t formatCount;
    //get format count with first call 
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

    if (formatCount != 0) 
    {//resize to count and populate with 2nd call   
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }

    //same as for format count
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) 
    {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

//chose surface format with suitable color format and color space
VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) 
{
    for (const auto& availableFormat : availableFormats) 
    {                                 
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) 
        {
            return availableFormat;
        }
    }
    return availableFormats[0];
}

//synchronisation setting (vsync, immediate or in this case, mailbox)
VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) 
{
    for (const auto& availablePresentMode : availablePresentModes) 
    {                           
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) 
        {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window) 
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) 
    {
        return capabilities.currentExtent;
    } else 
    {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}


bool checkDeviceExtensionSupport(VkPhysicalDevice device) 
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    //get list of available extensions
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    //make a set from the required extensions (defined at the start)
    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    //if all required extensions are there in the available extensions, the set is empty
    for (const auto& extension : availableExtensions) 
    {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

//function to check gpu validity
bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface) 
{
    QueueFamilyIndices indices = findQueueFamilies(device, surface);

    bool extensionsSupported = checkDeviceExtensionSupport(device);

    bool swapChainAdequate = false;
    if (extensionsSupported) 
    {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device, surface);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    return indices.isComplete() && extensionsSupported && swapChainAdequate;
}


//Used to create the debug messenger. Needed because the debug messenger is a vulkan extension (hence the EXT)
VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) 
{//                                                                                                                                                                       - vkGetInstanceProcAddr gives us a void pointer to a function which it gets by searching the graphics driver
//                                                                                                                                                                          for the whatever is passed as an argument. In this case, searching for "vkCreateDebugUtilsMessengerEXT"
//                                                                                                                                                                          yields a pointer to a function which is responsible for othe creation of the debug messenger
//                                                                                                                                                                          here, the void pointer is casted to PFN_vkCreateDebugUtilsMessengerEXT, and the function it points to is
//                                                                                                                                                                          called on some parameters, notably pDebugMessenger, which is the actual debug messenger later used to give
//               cast to this                         returns void pointer to function  after searching gpu driver                    what we are searching for             us debug messages
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)                    vkGetInstanceProcAddr                            (instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        //creation of debug messenger
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } 
    else 
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

//destroy the debug messenger
void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) 
{   
    //same logic as above but used to destroy the debug messenger in this case (used for cleanup)
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) 
    {
        func(instance, debugMessenger, pAllocator);
    }
}

//function to check validation layer support
bool check_validation_layer_support(const std::vector<const char*> validation_layers)
{
    //count of validation layers
    uint32_t layer_count;

    //function with nullptr just returns count to layer_count
    vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

    //vector of length layer_count to hold layers 
    std::vector<VkLayerProperties> available_layers(layer_count);
    
    //stores available layers to available_layers
    vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

    //check all layer names in validation_layers against those in available layers
    for (const char* layer_name : validation_layers)
    {
        bool layer_found = false;

        for (const auto& available_layer : available_layers)
        {
            if (strcmp(layer_name, available_layer.layerName) == 0)
            {
                layer_found = true;
                break;
            }
        }

        if (!layer_found)
        {
            return false;
        }
    }

    return true;
}

//function which actually prints the error message to screen. VKAPI_ATTR and VKAPI_CALL are C macros which are for vulkan convention
VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) 
{
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}

//populates the VkDebugUtilsMessengerCreateInfoEXT struct of which a pointer gets passed to the VkInstanceCreateInfo struct's .pNext field while creating a vulkan instance, also used to create the actual debug messenger using CreateDebugUtilsMessengerEXT
void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) 
{
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback; 
}

//creates debug messenger by calleing CreateDebugUtilsMessengerEXT after populating VkDebugUtilsMessengerCreateInfoEXT struct
void setupDebugMessenger(VkInstance instance, VkDebugUtilsMessengerEXT& debug_messenger) 
{
        if (!enableValidationLayers) return;

        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        populateDebugMessengerCreateInfo(createInfo);

        if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debug_messenger) != VK_SUCCESS) 
        {
            throw std::runtime_error("failed to set up debug messenger");
        }
}

void createSurface(VkInstance instance, GLFWwindow* window, VkSurfaceKHR& surface) 
{
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) 
    {
        throw std::runtime_error("failed to create window surface!");
    }
}

//function to get required extensions (using glfw)
std::vector<const char*> getRequiredExtensions() 
{
    if (!glfwInit())
    {
        throw std::runtime_error("tried to call glfwGetRequiredInstanceExtensions function without initialising glfw");
    }

    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (enableValidationLayers)
    {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

//read spir-v into buffer
std::vector<char> readFile(const std::string& filename) 
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) 
    {
        throw std::runtime_error("failed to open file!");
    }

    size_t fileSize = (size_t) file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
}

//create shader module
VkShaderModule createShaderModule(const std::vector<char>& code, VkDevice device) 
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();

    //spir-v is built on 32 bit words, basically this line says treat this array as being filled with 32 bit integers not 1 byte chars
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) 
    {
        throw std::runtime_error("failed to create shader module!");
    }

    return shaderModule;
}

//create vulkan instance without debug messenger
void create_instance_without_debug(VkInstance& instance, const char* app_name, uint32_t api_version)
{   
    //declaring and initialising app info struct 
    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = app_name;
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "No Engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = api_version;

    //getting required extensions
    auto extensions = getRequiredExtensions();

    //declaring and initialising create info struct 
    VkInstanceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;
    create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    create_info.ppEnabledExtensionNames = extensions.data();
    create_info.enabledLayerCount = 0;
    create_info.ppEnabledLayerNames = nullptr;

    //try to create instance and check for success
    if (vkCreateInstance(&create_info, nullptr, &instance) != VK_SUCCESS)
    {
        throw std::runtime_error("instance creation failed");
    }
}


//create instance with debug messenger
void create_instance(VkInstance& instance, const char* app_name, uint32_t api_version)
{
    
    if (enableValidationLayers && !check_validation_layer_support(validationLayers)) 
    {
        throw std::runtime_error("validation layers not available");
    }

    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = app_name;
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "No Engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = api_version;

    auto extensions = getRequiredExtensions();

    VkInstanceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;
    create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    create_info.ppEnabledExtensionNames = extensions.data();
    create_info.enabledLayerCount = 0;
    create_info.pNext = nullptr;

    if (vkCreateInstance(&create_info, nullptr, &instance) != VK_SUCCESS)
    {
        throw std::runtime_error("instance creation failed");
    }
}


//list and select physical device
void pick_physical_device(VkInstance instance, VkPhysicalDevice& physical_device, VkSurfaceKHR surface)
{
    uint32_t device_count = 0;

    //first call with arg as nullptr just returns count and not the actual device data
    vkEnumeratePhysicalDevices(instance, &device_count, nullptr);

    if (device_count == 0)
    {
        throw std::runtime_error("no gpu found with vulkan support");
    }

    std::vector<VkPhysicalDevice> devices(device_count);

    //second call fills devices 
    vkEnumeratePhysicalDevices(instance, &device_count, devices.data());

    for (const auto& device : devices)
    {
        if (isDeviceSuitable(device, surface))
        {
            physical_device = device;
            break;
        }
    }

    if (physical_device == VK_NULL_HANDLE)
    {
        throw std::runtime_error("could not find suitable GPU");
    }
}

void create_logical_device(VkPhysicalDevice physical_device, VkDevice& device, VkSurfaceKHR surface, VkQueue& graphics_queue, VkQueue& present_queue)
{
    QueueFamilyIndices indices = findQueueFamilies(physical_device, surface);

    //initialise vector of queue create infos
    std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
    std::set<uint32_t> unique_queue_families = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    //both graphics and present queues have same priority
    float queue_priority = 1.0f;

    for (uint32_t queue_family : unique_queue_families)
    {   //populate VkDeviceQueueCreateInfo struct and push back to the vector 
        VkDeviceQueueCreateInfo queue_create_info{};
        queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info.queueFamilyIndex = queue_family;
        queue_create_info.queueCount = 1;
        queue_create_info.pQueuePriorities = &queue_priority;
        queue_create_infos.push_back(queue_create_info);
    }


    VkPhysicalDeviceFeatures device_features{};

    //create and populate device create info with required information about queues
    VkDeviceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
    create_info.pQueueCreateInfos = queue_create_infos.data();

    create_info.pEnabledFeatures = &device_features;

    create_info.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    create_info.ppEnabledExtensionNames = deviceExtensions.data();

    create_info.enabledLayerCount = 0;
    
    //creating the actual logical device using create_info that was populated above
    if (vkCreateDevice(physical_device, &create_info, nullptr, &device) != VK_SUCCESS) 
    {
        throw std::runtime_error("failed to create logical device!");
    }  
    
    //gets memory handles to queues (pointers basically)
    vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphics_queue);
    vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &present_queue);
}

void create_swapchain(VkPhysicalDevice physical_device, VkDevice device, VkSurfaceKHR surface, GLFWwindow* window, VkSwapchainKHR& swap_chain, std::vector<VkImage>& swap_chain_images, VkFormat& swap_chain_image_format, VkExtent2D& swap_chain_extent)
{   
    SwapChainSupportDetails swapchain_support = querySwapChainSupport(physical_device, surface);

    //helper functions to get surface parameters
    VkSurfaceFormatKHR surface_format = chooseSwapSurfaceFormat(swapchain_support.formats);
    VkPresentModeKHR present_mode = chooseSwapPresentMode(swapchain_support.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapchain_support.capabilities, window);

    uint32_t image_count = 3;

    if (swapchain_support.capabilities.maxImageCount > 0 && image_count > swapchain_support.capabilities.maxImageCount)
    {
        image_count = swapchain_support.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR swapchain_create_info{};
    swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_create_info.surface = surface;
    swapchain_create_info.minImageCount = image_count;
    swapchain_create_info.imageFormat = surface_format.format;
    swapchain_create_info.imageColorSpace = surface_format.colorSpace;
    swapchain_create_info.imageExtent = extent;
    swapchain_create_info.imageArrayLayers = 1;
    swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = findQueueFamilies(physical_device, surface);
    uint32_t queue_family_indices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    if (indices.graphicsFamily != indices.presentFamily) 
    {
        swapchain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchain_create_info.queueFamilyIndexCount = 2;
        swapchain_create_info.pQueueFamilyIndices = queue_family_indices;
    } 
    else 
    {
        swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    swapchain_create_info.preTransform = swapchain_support.capabilities.currentTransform;
    swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_create_info.presentMode = present_mode;
    swapchain_create_info.clipped = VK_TRUE;
    swapchain_create_info.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(device, &swapchain_create_info, nullptr, &swap_chain) != VK_SUCCESS)
    {
        throw std::runtime_error("swapchain creation failed");
    }

    vkGetSwapchainImagesKHR(device, swap_chain, &image_count, nullptr);
    swap_chain_images.resize(image_count);
    vkGetSwapchainImagesKHR(device, swap_chain, &image_count, swap_chain_images.data());

    swap_chain_image_format = surface_format.format;
    swap_chain_extent = extent;

    std::cout << "swap chain image count: " << image_count << std::endl;
}

//create image views for each image in the swap chain images (cant directly interact with the image, so interact through the image views)
void createImageViews(std::vector<VkImageView>& swap_chain_image_views, std::vector<VkImage> swap_chain_images, VkFormat swap_chain_image_format, VkDevice device) 
{
    swap_chain_image_views.resize(swap_chain_images.size());

    for (size_t i = 0; i < swap_chain_images.size(); i++) 
    {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = swap_chain_images[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = swap_chain_image_format;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        //figure this out when i need to
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device, &createInfo, nullptr, &swap_chain_image_views[i]) != VK_SUCCESS) 
        {
            throw std::runtime_error("failed to create image views");
        }
    }
}

//create render pass
void create_render_pass(VkFormat swap_chain_color_format, VkDevice device, VkRenderPass& render_pass, VkFormat depth_format, VkSampleCountFlagBits samples)
{
    VkAttachmentDescription color_attachment{};
    color_attachment.format = swap_chain_color_format;
    color_attachment.samples = samples;
    //loadOp and storeOp are memory instructions which control what happens before (loadOp) the drawing and after (storeOp)
                                //fresh background every frame
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                                //save rendered pixels to memory
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                                        //temporary buffer 
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachment_ref{};
    //important: this is the out color in the fragment shader (like in opengl when you say layou(location = 0) out vec4 outColor;, that location = 0 is the color_attachment_reference.attachment)
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    //attachment for depth info (for depth buffer)
    VkAttachmentDescription depth_attachment{};
    depth_attachment.format = depth_format;
    depth_attachment.samples = samples;
    depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; 
    depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depth_attachment_ref{};
    //attachment is 1, and color attachment is 0
    depth_attachment_ref.attachment = 1; 
    depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    //subpass is what goes on within a render pass, one render pass may have one or more subpasses, here there's only one subpass
    //each subpass can have more than one attachment, however if there are actions which may cause read after write dependencies, then put them in separate subpasses which occur sequentially 
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;
    subpass.pDepthStencilAttachment = &depth_attachment_ref;

    //struct to control sequential order of subpasses (if neeeded)
    VkSubpassDependency dependency{};
    //source subpass, that is, taking an image from swap chain 
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    //destination subpass, that is subpass 0 (drawing)
    dependency.dstSubpass = 0;
    
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    std::vector<VkAttachmentDescription> attachments = {color_attachment, depth_attachment};



    VkRenderPassCreateInfo render_pass_create_info{};
    render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_create_info.attachmentCount = static_cast<uint32_t>(attachments.size());
    render_pass_create_info.pAttachments = attachments.data();
    render_pass_create_info.subpassCount = 1;
    render_pass_create_info.pSubpasses = &subpass;
    render_pass_create_info.dependencyCount = 1;
    render_pass_create_info.pDependencies = &dependency;

    if (vkCreateRenderPass(device, &render_pass_create_info, nullptr, &render_pass) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create render pass");
    }
}

uint32_t find_memory_type(VkPhysicalDevice physical_device, uint32_t type_filter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memory_properties{};
    vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);

    for (uint32_t i = 0; i < memory_properties.memoryTypeCount; i++)
    {
        if ((type_filter & (1 << i)) && (memory_properties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type");
}

void create_vertex_buffer(VkDevice device, VkPhysicalDevice physical_device, VkSurfaceKHR surface, std::vector<Vertex> vertices, VkBuffer& vertex_buffer, VkDeviceMemory& vertex_buffer_memory)
{
    VkBufferCreateInfo buffer_create_info{};
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.size = sizeof(vertices[0]) * vertices.size();
    buffer_create_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &buffer_create_info, nullptr, &vertex_buffer) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create vertex buffer");
    }

    VkMemoryAllocateInfo memory_allocate_info{};
    memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    
    VkMemoryRequirements memory_requirements{};
    vkGetBufferMemoryRequirements(device, vertex_buffer, &memory_requirements);
    uint32_t memory_type_index = find_memory_type(physical_device, memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    memory_allocate_info.memoryTypeIndex = memory_type_index;
    memory_allocate_info.allocationSize = memory_requirements.size;

    if (vkAllocateMemory(device, &memory_allocate_info, nullptr, &vertex_buffer_memory) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate memory");
    }

    if (vkBindBufferMemory(device, vertex_buffer, vertex_buffer_memory, 0) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to bind buffer");
    }

    void* data;

    if (vkMapMemory(device, vertex_buffer_memory, 0, sizeof(vertices[0]) * vertices.size(), 0, &data) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to map memory");
    }

    memcpy(data, vertices.data(), size_t(buffer_create_info.size));

    vkUnmapMemory(device, vertex_buffer_memory);
}

void create_uniform_buffer(VkDevice device, VkPhysicalDevice physical_device, std::vector<VkBuffer>& uniform_buffers, std::vector<VkDeviceMemory>& uniform_buffers_memory, std::vector<void*>& uniform_buffers_mapped, int max_frames_in_flight)
{
    VkDeviceSize buffer_size = sizeof(UniformBufferObject);
    uniform_buffers.resize(max_frames_in_flight);
    uniform_buffers_mapped.resize(max_frames_in_flight);
    uniform_buffers_memory.resize(max_frames_in_flight);

    for (size_t i = 0; i < max_frames_in_flight; i++)
    {
        VkBufferCreateInfo buffer_create_info{};
        buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_create_info.size = buffer_size;

        buffer_create_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(device, &buffer_create_info, nullptr, &uniform_buffers[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create uniform buffers");
        }

        VkMemoryRequirements memory_requirements{};
        vkGetBufferMemoryRequirements(device, uniform_buffers[i], &memory_requirements);

        VkMemoryAllocateInfo memory_allocate_info{};
        memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memory_allocate_info.allocationSize = memory_requirements.size;
        memory_allocate_info.memoryTypeIndex = find_memory_type(physical_device, memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        if (vkAllocateMemory(device, &memory_allocate_info, nullptr, &uniform_buffers_memory[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to allocate memory for uniform buffers");
        }

        vkBindBufferMemory(device, uniform_buffers[i], uniform_buffers_memory[i], 0);

        vkMapMemory(device, uniform_buffers_memory[i], 0, buffer_size, 0, &uniform_buffers_mapped[i]);
    }
}

void create_descriptor_set_layout(VkDevice device, VkDescriptorSetLayout& descriptor_set_layout) 
{
    VkDescriptorSetLayoutBinding ubo_layout_binding{};
    ubo_layout_binding.binding = 0;
    ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ubo_layout_binding.descriptorCount = 1;
    ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; 
    ubo_layout_binding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = 1;
    layout_info.pBindings = &ubo_layout_binding;

    if (vkCreateDescriptorSetLayout(device, &layout_info, nullptr, &descriptor_set_layout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout");
    }
}

void create_descriptor_pool(VkDevice device, VkDescriptorPool& descriptor_pool, int max_frames_in_flight)
{
    VkDescriptorPoolSize pool_size{};
    pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pool_size.descriptorCount = static_cast<uint32_t>(max_frames_in_flight);

    VkDescriptorPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.poolSizeCount = 1;
    pool_info.pPoolSizes = &pool_size;
    pool_info.maxSets = static_cast<uint32_t>(max_frames_in_flight);

    if (vkCreateDescriptorPool(device, &pool_info, nullptr, &descriptor_pool) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create descriptor sets");
    }
}

void create_descriptor_sets(VkDevice device, VkDescriptorSetLayout descriptor_set_layout, VkDescriptorPool descriptor_pool, std::vector<VkDescriptorSet>& descriptor_sets, std::vector<VkBuffer>& uniform_buffers ,int max_frames_in_flight)
{
    std::vector<VkDescriptorSetLayout> layouts(max_frames_in_flight, descriptor_set_layout);

    VkDescriptorSetAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = descriptor_pool;
    alloc_info.descriptorSetCount = static_cast<uint32_t>(max_frames_in_flight);
    alloc_info.pSetLayouts = layouts.data();

    descriptor_sets.resize(max_frames_in_flight);

    if (vkAllocateDescriptorSets(device, &alloc_info, descriptor_sets.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate descriptor sets");
    }

    for (size_t i = 0; i < max_frames_in_flight; i++) 
    {
        VkDescriptorBufferInfo buffer_info{};
        buffer_info.buffer = uniform_buffers[i];
        buffer_info.offset = 0;
        buffer_info.range = sizeof(UniformBufferObject);

        VkWriteDescriptorSet descriptor_write{};
        descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_write.dstSet = descriptor_sets[i];
        descriptor_write.dstBinding = 0;
        descriptor_write.dstArrayElement = 0;
        descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptor_write.descriptorCount = 1;
        descriptor_write.pBufferInfo = &buffer_info;

        vkUpdateDescriptorSets(device, 1, &descriptor_write, 0, nullptr);
    }
}

void create_graphics_pipeline(const std::string vertex_shader_filepath, const std::string fragment_shader_filepath, VkDevice device, VkPipelineLayout& pipeline_layout, VkRenderPass render_pass, VkPipeline& graphics_pipeline, VkDescriptorSetLayout& descriptor_set_layout)
{   //read the compiled spir-v shader files into buffers
    std::vector<char> vertex_shader_code = readFile(vertex_shader_filepath);
    std::vector<char> fragment_shader_code = readFile(fragment_shader_filepath);

    //create shader modules
    VkShaderModule vertex_shader_module = createShaderModule(vertex_shader_code, device);
    VkShaderModule fragment_shader_module = createShaderModule(fragment_shader_code, device);

    //create info structs for creating shader stages
    VkPipelineShaderStageCreateInfo vertex_shader_stage_create_info{};
    vertex_shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertex_shader_stage_create_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertex_shader_stage_create_info.module = vertex_shader_module;
    vertex_shader_stage_create_info.pName = "main";

    VkPipelineShaderStageCreateInfo fragment_shader_stage_create_info{};
    fragment_shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragment_shader_stage_create_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragment_shader_stage_create_info.module = fragment_shader_module;
    fragment_shader_stage_create_info.pName = "main";

    VkPipelineShaderStageCreateInfo shader_stage_create_infos[] = {vertex_shader_stage_create_info, fragment_shader_stage_create_info};

    //vertex buffer at binding 0 with stride sizeof(Vertex), inputRate says move forward by one stride after each vertex being drawn
    VkVertexInputBindingDescription vertex_input_binding_description{};
    vertex_input_binding_description.binding = 0;
    vertex_input_binding_description.stride = static_cast<uint32_t>(sizeof(Vertex));    
    vertex_input_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::vector<VkVertexInputBindingDescription> vertex_input_binding_descriptions = {vertex_input_binding_description};

    //attribute structs for position and color, basically within the Vertex, where is position and where is color, both bound to 0
    VkVertexInputAttributeDescription position_input_attribute_description{};
    position_input_attribute_description.location = 0;
    position_input_attribute_description.binding = 0;
    position_input_attribute_description.offset = static_cast<uint32_t>(offsetof(Vertex, position));
    position_input_attribute_description.format = VK_FORMAT_R32G32B32_SFLOAT;
    
    VkVertexInputAttributeDescription color_input_attribute_description{};
    color_input_attribute_description.location = 1;
    color_input_attribute_description.binding = 0;
    color_input_attribute_description.offset = static_cast<uint32_t>(offsetof(Vertex, color));
    color_input_attribute_description.format = VK_FORMAT_R32G32B32_SFLOAT;

    VkVertexInputAttributeDescription normal_input_attribute_description{};
    normal_input_attribute_description.location = 2;
    normal_input_attribute_description.binding = 0;
    normal_input_attribute_description.offset = static_cast<uint32_t>(offsetof(Vertex, normal));
    normal_input_attribute_description.format = VK_FORMAT_R32G32B32_SFLOAT;

    std::vector<VkVertexInputAttributeDescription> vertex_input_attribute_descriptions = {position_input_attribute_description, color_input_attribute_description, normal_input_attribute_description};
    
    //VkPipelineVertexInputStateCreateInfo contains the arrays with VkVertexInputAttributeDescription and VkVertexInputBindingDescription structs
    VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info{};
    vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    vertex_input_state_create_info.vertexBindingDescriptionCount = 1;
    vertex_input_state_create_info.pVertexBindingDescriptions = vertex_input_binding_descriptions.data();

    vertex_input_state_create_info.vertexAttributeDescriptionCount = 3;
    vertex_input_state_create_info.pVertexAttributeDescriptions = vertex_input_attribute_descriptions.data();

    VkPipelineInputAssemblyStateCreateInfo input_assembly{};
    input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    //topology
    input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewport_state{};
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    //polygon mode (fill, line, point etc)
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState color_blend_attachment{};
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = VK_FALSE;    

    VkPipelineColorBlendStateCreateInfo color_blending_create_info{};
    color_blending_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blending_create_info.logicOpEnable = VK_FALSE;
    color_blending_create_info.logicOp = VK_LOGIC_OP_COPY;
    color_blending_create_info.attachmentCount = 1;
    color_blending_create_info.pAttachments = &color_blend_attachment;
    color_blending_create_info.blendConstants[0] = 0.0f;
    color_blending_create_info.blendConstants[1] = 0.0f;
    color_blending_create_info.blendConstants[2] = 0.0f;
    color_blending_create_info.blendConstants[3] = 0.0f;

    VkPipelineDepthStencilStateCreateInfo depth_stencil{};
    depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil.depthTestEnable = VK_TRUE;           
    depth_stencil.depthWriteEnable = VK_TRUE;          
    depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS; 
    depth_stencil.depthBoundsTestEnable = VK_FALSE;
    depth_stencil.stencilTestEnable = VK_FALSE;


    std::vector<VkDynamicState> dynamic_states = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamic_state{};
    dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
    dynamic_state.pDynamicStates = dynamic_states.data();

    VkPushConstantRange push_constant{};
    push_constant.offset = 0;
    push_constant.size = sizeof(PushConstantData);
    push_constant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    //used to describe the data that you want to send to the gpu, like push constants (small) or descriptor (big but not very big)
    VkPipelineLayoutCreateInfo pipeline_layout_info{};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 1;
    pipeline_layout_info.pSetLayouts = &descriptor_set_layout;
    pipeline_layout_info.pushConstantRangeCount = 1;
    pipeline_layout_info.pPushConstantRanges = &push_constant;

    if (vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr, &pipeline_layout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create pipeline layout");
    }

    VkGraphicsPipelineCreateInfo pipeline_create_info{};
    pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_create_info.stageCount = 2;
    pipeline_create_info.pStages = shader_stage_create_infos;
    pipeline_create_info.pVertexInputState = &vertex_input_state_create_info;
    pipeline_create_info.pInputAssemblyState = &input_assembly;
    pipeline_create_info.pViewportState = &viewport_state;
    pipeline_create_info.pRasterizationState = &rasterizer;
    pipeline_create_info.pMultisampleState = &multisampling;
    pipeline_create_info.pColorBlendState = &color_blending_create_info;
    pipeline_create_info.pDepthStencilState = &depth_stencil;
    pipeline_create_info.pDynamicState = &dynamic_state;
    pipeline_create_info.layout = pipeline_layout;
    pipeline_create_info.renderPass = render_pass;
    pipeline_create_info.subpass = 0;
    pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &graphics_pipeline) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create graphics pipeline");
    }

    vkDestroyShaderModule(device, vertex_shader_module, nullptr);
    vkDestroyShaderModule(device, fragment_shader_module, nullptr);
}


void create_frame_buffers(std::vector<VkFramebuffer>& swap_chain_frame_buffers, std::vector<VkImageView> swap_chain_image_views, VkRenderPass render_pass, VkExtent2D swap_chain_extent, VkDevice device, const std::vector<VkImageView>& depth_image_views) 
{
    swap_chain_frame_buffers.resize(swap_chain_image_views.size());

    for (size_t i = 0; i < swap_chain_image_views.size(); i++)
    {
        
        std::vector<VkImageView> attachments = {swap_chain_image_views[i], depth_image_views[i]};

        VkFramebufferCreateInfo frame_buffer_create_info{};
        frame_buffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        frame_buffer_create_info.renderPass = render_pass;
        frame_buffer_create_info.attachmentCount = static_cast<uint32_t>(attachments.size());
        frame_buffer_create_info.pAttachments = attachments.data();
        frame_buffer_create_info.width = swap_chain_extent.width;
        frame_buffer_create_info.height = swap_chain_extent.height;
        frame_buffer_create_info.layers = 1;

        if (vkCreateFramebuffer(device, &frame_buffer_create_info, nullptr, &swap_chain_frame_buffers[i]) != VK_SUCCESS)
        {
            throw std::runtime_error(std::format("failed to create frame buffer {}", i));
        }
    }
}

void create_command_pool(VkPhysicalDevice physical_device, VkDevice device, VkSurfaceKHR surface, VkCommandPool& command_pool)
{
    QueueFamilyIndices queue_families = findQueueFamilies(physical_device, surface);

    VkCommandPoolCreateInfo command_pool_create_info{};
    command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    command_pool_create_info.queueFamilyIndex = queue_families.graphicsFamily.value();

    if (vkCreateCommandPool(device, &command_pool_create_info, nullptr, &command_pool) != VK_SUCCESS) 
    {
            throw std::runtime_error("failed to create command pool");
    }
}

void create_command_buffers(VkCommandPool command_pool, VkDevice device, std::vector<VkCommandBuffer>& command_buffers)
{
    VkCommandBufferAllocateInfo command_buffer_allocate_info{};
    command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocate_info.commandPool = command_pool;
    command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_allocate_info.commandBufferCount = (uint32_t)command_buffers.size();

    if (vkAllocateCommandBuffers(device, &command_buffer_allocate_info, command_buffers.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create command buffer");
    }
}

void record_command_buffer(uint32_t image_index, VkCommandBuffer command_buffer, VkRenderPass render_pass, const std::vector<VkFramebuffer>& swap_chain_frame_buffers, VkExtent2D swap_chain_extent, VkPipeline graphics_pipeline, const std::vector<RenderObject>& render_objects, VkDescriptorSet& descriptor_set, VkPipelineLayout pipeline_layout)
{
    VkCommandBufferBeginInfo command_buffer_begin_info{};
    command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to begin command buffer");
    }

    std::array<VkClearValue, 2> clear_values{};

    clear_values[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clear_values[1].depthStencil = {1.0f, 0};

    VkRenderPassBeginInfo render_pass_begin_info{};
    render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
    render_pass_begin_info.pClearValues = clear_values.data();
    render_pass_begin_info.renderPass = render_pass;
    render_pass_begin_info.framebuffer = swap_chain_frame_buffers[image_index];
    render_pass_begin_info.renderArea.offset = {0, 0};
    render_pass_begin_info.renderArea.extent = swap_chain_extent;

    vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);

    VkViewport viewport{};viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swap_chain_extent.width);
    viewport.height = static_cast<float>(swap_chain_extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(command_buffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swap_chain_extent;
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);

    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_set, 0, nullptr);

    for (const auto& object: render_objects)
    {
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(command_buffer, 0, 1, &object.vertex_buffer, offsets);

        vkCmdBindIndexBuffer(command_buffer, object.index_buffer, 0, VK_INDEX_TYPE_UINT16);

        vkCmdPushConstants(command_buffer, pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstantData), &object.push_constants);

        vkCmdDrawIndexed(command_buffer, object.index_count, 1, 0, 0, 0);  
    }

    vkCmdEndRenderPass(command_buffer);

    if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to record command buffer");
    }
}

void create_sync_objects(VkDevice device, std::vector<VkSemaphore>& image_available_semaphores, std::vector<VkSemaphore>& render_finished_semaphores, std::vector<VkFence>& in_flight_fences, int max_frames_in_flight)
{   
    image_available_semaphores.resize(max_frames_in_flight);
    render_finished_semaphores.resize(max_frames_in_flight);
    in_flight_fences.resize(max_frames_in_flight);


    VkSemaphoreCreateInfo semaphore_create_info{};
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence_create_info{};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < max_frames_in_flight; i++)
    {
        if (vkCreateSemaphore(device, &semaphore_create_info, nullptr, &image_available_semaphores[i]) != VK_SUCCESS || 
            vkCreateSemaphore(device, &semaphore_create_info, nullptr, &render_finished_semaphores[i]) != VK_SUCCESS ||
            vkCreateFence(device, &fence_create_info, nullptr, &in_flight_fences[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create sync objects");
        }
    }
}

void draw_frame(uint32_t current_frame, VkDevice device, std::vector<VkFence>& in_flight_fences, std::vector<VkCommandBuffer>& command_buffers, VkRenderPass render_pass, const std::vector<VkFramebuffer>& swap_chain_frame_buffers, VkExtent2D swap_chain_extent, VkPipeline graphics_pipeline, const std::vector<RenderObject>& render_objects, VkSwapchainKHR swap_chain, std::vector<VkSemaphore>& image_available_semaphores, std::vector<VkSemaphore>& render_finished_semaphores, VkQueue graphics_queue, VkDescriptorSet& descriptor_set, VkPipelineLayout pipeline_layout)
{
    vkWaitForFences(device, 1, &in_flight_fences[current_frame], VK_TRUE, UINT64_MAX);
    
    uint32_t image_index;
    vkAcquireNextImageKHR(device, swap_chain, UINT64_MAX, image_available_semaphores[current_frame], VK_NULL_HANDLE, &image_index);

    vkResetFences(device, 1, &in_flight_fences[current_frame]);

    vkResetCommandBuffer(command_buffers[current_frame], 0);
    record_command_buffer(image_index, command_buffers[current_frame], render_pass, swap_chain_frame_buffers, swap_chain_extent, graphics_pipeline, render_objects, descriptor_set, pipeline_layout);

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore wait_semaphores[] = {image_available_semaphores[current_frame]};
    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = wait_semaphores;
    submit_info.pWaitDstStageMask = wait_stages;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffers[current_frame];

    VkSemaphore signal_semaphores[] = {render_finished_semaphores[current_frame]};
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = signal_semaphores;

    if (vkQueueSubmit(graphics_queue, 1, &submit_info, in_flight_fences[current_frame]) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to submit draw command buffer");
    }

    VkPresentInfoKHR present_info{};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &render_finished_semaphores[current_frame];
    
    VkSwapchainKHR swap_chains[] = {swap_chain};
    present_info.swapchainCount = 1;
    present_info.pSwapchains = swap_chains;
    present_info.pImageIndices = &image_index;

    vkQueuePresentKHR(graphics_queue, &present_info);
}

void cleanup(VkDevice device, std::vector<VkSemaphore> render_finished_semaphores, std::vector<VkSemaphore> image_available_semaphores, std::vector<VkFence> in_flight_fences, VkCommandPool command_pool, std::vector<VkFramebuffer> swap_chain_frame_buffers, VkPipeline graphics_pipeline, VkPipelineLayout pipeline_layout, VkRenderPass render_pass, std::vector<VkImageView> swap_chain_image_views, VkSwapchainKHR swap_chain, VkDebugUtilsMessengerEXT debug_messenger, VkSurfaceKHR surface, VkInstance instance, GLFWwindow* window, std::vector<VkBuffer> uniform_buffers, std::vector<VkDeviceMemory> uniform_buffers_memory, VkDescriptorPool descriptor_pool, VkDescriptorSetLayout descriptor_set_layout, int max_frames_in_flight, std::vector<VkImageView> depth_image_views, std::vector<VkImage> depth_images, std::vector<VkDeviceMemory> depth_image_memories, std::vector<RenderObject> render_objects)
{
    for (int i = 0; i < max_frames_in_flight; i++)
    {
        vkDestroySemaphore(device, render_finished_semaphores[i], nullptr);
        vkDestroySemaphore(device, image_available_semaphores[i], nullptr);
        vkDestroyFence(device, in_flight_fences[i], nullptr);

        vkDestroyBuffer(device, uniform_buffers[i], nullptr);
        vkFreeMemory(device, uniform_buffers_memory[i], nullptr);
    }

    

    vkDestroyDescriptorPool(device, descriptor_pool, nullptr);

    vkDestroyDescriptorSetLayout(device, descriptor_set_layout, nullptr);

    vkDestroyCommandPool(device, command_pool, nullptr);


    for (auto frame_buffer : swap_chain_frame_buffers)
    {
        vkDestroyFramebuffer(device, frame_buffer, nullptr);
    }

    vkDestroyPipeline(device, graphics_pipeline, nullptr);
    vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
    vkDestroyRenderPass(device, render_pass, nullptr);

    for (size_t i = 0; i < swap_chain_image_views.size(); i++)
    {
        vkDestroyImageView(device, depth_image_views[i], nullptr);
        vkDestroyImage(device, depth_images[i], nullptr);
        vkFreeMemory(device, depth_image_memories[i], nullptr);
    }
    
    for (auto imageView : swap_chain_image_views) {
        vkDestroyImageView(device, imageView, nullptr);
    }

    vkDestroySwapchainKHR(device, swap_chain, nullptr);

    for (auto object: render_objects)
    {
        vkDestroyBuffer(device, object.index_buffer, nullptr);
        vkFreeMemory(device, object.index_buffer_memory, nullptr);
        
        vkDestroyBuffer(device, object.vertex_buffer, nullptr);
        vkFreeMemory(device, object.vertex_buffer_memory, nullptr);
    }

    vkDestroyDevice(device, nullptr);

    if (enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(instance, debug_messenger, nullptr);
    }

    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);

    glfwDestroyWindow(window);
}

VkFormat find_supported_format(VkPhysicalDevice physical_device, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) 
{
    for (VkFormat format : candidates) 
    {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physical_device, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) return format;
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) return format;
    }
    throw std::runtime_error("failed to find supported format!");
}

VkFormat find_depth_format(VkPhysicalDevice physical_device) 
{
    return find_supported_format(
        physical_device,
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

void create_image(VkDevice device, VkPhysicalDevice physical_device, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& image_memory, VkSampleCountFlagBits samples, VkSharingMode sharing_mode) 
{
    VkImageCreateInfo image_info{};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.extent.width = width;
    image_info.extent.height = height;
    image_info.extent.depth = 1;
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.format = format;
    image_info.tiling = tiling;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.usage = usage;
    image_info.samples = samples;
    image_info.sharingMode = sharing_mode;

    if (vkCreateImage(device, &image_info, nullptr, &image) != VK_SUCCESS) throw std::runtime_error("failed to create image");

    VkMemoryRequirements mem_requirements;
    vkGetImageMemoryRequirements(device, image, &mem_requirements);

    VkMemoryAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_requirements.size;
    alloc_info.memoryTypeIndex = find_memory_type(physical_device, mem_requirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &alloc_info, nullptr, &image_memory) != VK_SUCCESS) throw std::runtime_error("failed to allocate image memory");

    vkBindImageMemory(device, image, image_memory, 0);
}

VkImageView create_image_view(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspect_flags) 
{
    VkImageViewCreateInfo view_info{};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image = image;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = format;
    view_info.subresourceRange.aspectMask = aspect_flags;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = 1;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 1;

    VkImageView image_view;
    if (vkCreateImageView(device, &view_info, nullptr, &image_view) != VK_SUCCESS) throw std::runtime_error("failed to create image view!");
    return image_view;
}
