#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <vector>
#include <stdexcept>

std::vector<const char*> getRequiredExtensions() {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        return extensions;
    }

int main()
{   
    //init glfw
    if (!glfwInit()) 
    {
        throw std::runtime_error("failed to initialize GLFW");
    }

    //declaring instance
    VkInstance instance;

    //declaring and initialising app info struct (the optional one)
    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Hello Triangle";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "No Engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_4;
    
    //getting required extensions
    auto extensions = getRequiredExtensions();

    //declaring and initialising create info struct (not the optional one)
    VkInstanceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;
    create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    create_info.ppEnabledExtensionNames = extensions.data();
    create_info.enabledLayerCount = 0;
    create_info.ppEnabledLayerNames = nullptr;

    //check if instance creation was successful
    if (vkCreateInstance(&create_info, nullptr, &instance) != VK_SUCCESS)
    {
        throw std::runtime_error("instance creation failed");
    }

    //count of physical devices, ie count of gpus
    uint32_t device_count = 0;
    //get gpu count (since the last argument is null it doesnt try giving the actual list of gpus)
    vkEnumeratePhysicalDevices(instance, &device_count, nullptr);

    //check if supported gpus appear
    if (device_count == 0)
    {
        throw std::runtime_error("no vulkan supported gpus");
    }
    //vector of available gpus of length device count 
    std::vector<VkPhysicalDevice> devices(device_count);

    //get gpus into the devices vector
    vkEnumeratePhysicalDevices(instance, &device_count, devices.data());

    int i = 0;
    for (const auto& device : devices)
    {
        //device properties struct  declaration
        VkPhysicalDeviceProperties2 device_properties{};
        device_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;

        //function to populate members of device_properties
        vkGetPhysicalDeviceProperties2(device, &device_properties);

        std::cout << "Device " << i << std::endl;
        std::cout << "API Version: " << device_properties.properties.apiVersion << std::endl;
        std::cout << "Driver Version: " << device_properties.properties.driverVersion << std::endl;
        std::cout << "Vendor ID: " << std::hex << device_properties.properties.vendorID << std::dec << std::endl;
        std::cout << "Device Name: " << device_properties.properties.deviceName << std::endl;
        if (device_properties.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) std::cout << "Device type: Discrete GPU" << std::endl;
        else std::cout << "Device type: Integrated GPU" << std::endl;
        printf("\n");
        
        //declaration of device memory properties struct
        VkPhysicalDeviceMemoryProperties2 device_memory_properties{};
        device_memory_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;

        //function to populate device_memory_properties
        vkGetPhysicalDeviceMemoryProperties2(device, &device_memory_properties);

        std::cout << "Memory properties: " << std::endl;
        std::cout << "Number of memory heaps (vram + regular ram given to gpu): " << device_memory_properties.memoryProperties.memoryHeapCount << std::endl;
    
        for (uint32_t heap_count = 0; heap_count < device_memory_properties.memoryProperties.memoryHeapCount; heap_count++)
        {
            std::cout << "Size of heap " << heap_count << ": " << device_memory_properties.memoryProperties.memoryHeaps[heap_count].size << " bytes" << std::endl;            
        }
        printf("\n\n");
        i++;
    }
    vkDestroyInstance(instance, nullptr);
    glfwTerminate();

    return 0;
}