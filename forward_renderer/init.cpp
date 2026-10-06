#include "init.hpp"

void Renderer::create_instance()
{
    if (enableValidationLayers && !checkValidationLayerSupport()) 
    {
        throw std::runtime_error("validation layers not available");
    }

    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "forward_renderer";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "No Engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_4;

    auto extensions = getRequiredExtensions();

    VkInstanceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;
    create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    create_info.ppEnabledExtensionNames = extensions.data();

    VkDebugUtilsMessengerCreateInfoEXT dbg{};
    VkValidationFeatureEnableEXT enabled_features[] = {VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT};
    VkValidationFeaturesEXT validation_features{};
    if (enableValidationLayers)
    {
        create_info.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        create_info.ppEnabledLayerNames = validationLayers.data();
        
        
        validation_features.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
        validation_features.enabledValidationFeatureCount = 1;
        validation_features.pEnabledValidationFeatures = enabled_features;
        validation_features.pNext = &dbg;   

        populateDebugMessengerCreateInfo(dbg);
        create_info.pNext = &validation_features;
    }
    else
    {
        create_info.enabledLayerCount = 0;
        create_info.ppEnabledLayerNames = nullptr;
        create_info.pNext = nullptr;
    }
    
    if (vkCreateInstance(&create_info, nullptr, &instance) != VK_SUCCESS)
    {
        throw std::runtime_error("instance creation failed");
    }

    resources.push_back(ResourceTypes::Instance, instance);
}

void Renderer::create_surface() 
{
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }

    resources.push_back(ResourceTypes::Surface, surface);
}

void Renderer::pick_physical_device()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    for (const auto& device : devices) {
        if (isDeviceSuitable(device)) {
            physical_device = device;
            break;
        }
    }

    if (physical_device == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to find a suitable GPU!");
    }
}

void Renderer::create_memory_allocator()
{
    VmaAllocatorCreateInfo allocator_info{};
    allocator_info.device = device;
    allocator_info.physicalDevice = physical_device;
    allocator_info.instance = instance;

    if (vmaCreateAllocator(&allocator_info, &allocator) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create memory allocator");
    }

    resources.push_back(ResourceTypes::Allocator, allocator);
}

void Renderer::create_logical_device() 
{
    QueueFamilyIndices indices = findQueueFamilies(physical_device);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();

    createInfo.pEnabledFeatures = &deviceFeatures;

    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(physical_device, &createInfo, nullptr, &device) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }

    vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphics_queue);
    vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &present_queue);

    resources.push_back(ResourceTypes::LogicalDevice, device);
}

