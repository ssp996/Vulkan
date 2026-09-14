#include "vulkan_compute.hpp"

void create_compute_pipeline(const std::string& shader_filepath, VkPipelineLayout& pipeline_layout, VkPipeline& compute_pipeline, const std::vector<VkDescriptorSetLayout>& descriptor_set_layouts, VkDevice device, const std::vector<VkPushConstantRange>& push_constant_ranges)
{
    std::vector<char> shader_code = readFile(shader_filepath);
    VkShaderModule compute_shader_module = createShaderModule(shader_code, device);

    VkPipelineLayoutCreateInfo pipeline_layout_create_info{};
    pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_create_info.pushConstantRangeCount = static_cast<uint32_t>(push_constant_ranges.size());
    pipeline_layout_create_info.pPushConstantRanges = push_constant_ranges.data();
    pipeline_layout_create_info.setLayoutCount = static_cast<uint32_t>(descriptor_set_layouts.size());
    pipeline_layout_create_info.pSetLayouts = descriptor_set_layouts.data();

    if (vkCreatePipelineLayout(device, &pipeline_layout_create_info, nullptr, &pipeline_layout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create pipeline layout");   
    }

    VkPipelineShaderStageCreateInfo shader_stage_create_info{};
    shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stage_create_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    shader_stage_create_info.pName = "main";
    shader_stage_create_info.module = compute_shader_module;

    VkComputePipelineCreateInfo pipeline_create_info{};
    pipeline_create_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipeline_create_info.stage = shader_stage_create_info;
    pipeline_create_info.layout = pipeline_layout;

    if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &compute_pipeline) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create compute pipeline");
    }

    vkDestroyShaderModule(device, compute_shader_module, nullptr);
}

int is_compute_ready(VkPhysicalDevice physical_device)
{
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr); 

    std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);

    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_families.data()); 
    
    std::optional<uint32_t> queue_family_index;

    int i = 0;
    for (const auto& queue_family: queue_families)
    {
        if (queue_family.queueFlags & VK_QUEUE_COMPUTE_BIT && !(queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT))
        {
            queue_family_index = i;
            break;
        }
        i++;
    }

    if (!queue_family_index.has_value())
    {
        return -1;
    }
    
    return queue_family_index.value();
}

void create_compute_device(VkPhysicalDevice physical_device, VkDevice& device, VkQueue& compute_queue)
{
    uint32_t queue_family_index = is_compute_ready(physical_device);

    if (queue_family_index == -1)
    {
        throw std::runtime_error("no dedicated compute queue family found");
    }

    float compute_queue_priority = 1.0f;

    VkDeviceQueueCreateInfo queue_create_info{};
    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.queueFamilyIndex = static_cast<uint32_t>(queue_family_index);
    queue_create_info.queueCount = 1;
    queue_create_info.pQueuePriorities = &compute_queue_priority;

    VkPhysicalDeviceFeatures physical_device_features{};

    VkDeviceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    create_info.queueCreateInfoCount = 1;
    create_info.pQueueCreateInfos = &queue_create_info;

    create_info.pEnabledFeatures = &physical_device_features;

    create_info.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    create_info.ppEnabledExtensionNames = deviceExtensions.data();

    create_info.enabledLayerCount = 0;
    
    if (vkCreateDevice(physical_device, &create_info, nullptr, &device) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create logical device");
    }

    vkGetDeviceQueue(device, queue_family_index, 0, &compute_queue);
}   

void pick_compute_physical_device(VkInstance instance, VkPhysicalDevice& physical_device)
{
    uint32_t device_count = 0;

    vkEnumeratePhysicalDevices(instance, &device_count, nullptr);

    if (device_count == 0)
    {
        throw std::runtime_error("no gpu found with vulkan support");
    }

    std::vector<VkPhysicalDevice> devices(device_count);

    vkEnumeratePhysicalDevices(instance, &device_count, devices.data());

    for (const auto& device: devices)
    {
        if (is_compute_ready(device))
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