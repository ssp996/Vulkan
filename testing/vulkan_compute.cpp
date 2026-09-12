#include "vulkan_compute.hpp"

void create_compute_pipeline(const std::string& shader_filepath, VkPipelineLayout& pipeline_layout, VkPipeline& compute_pipeline, const std::vector<VkDescriptorSetLayout>& descriptor_set_layouts, VkDevice device)
{
    std::vector<char> shader_code = readFile(shader_filepath);
    VkShaderModule compute_shader_module = createShaderModule(shader_code, device);

    VkPipelineLayoutCreateInfo pipeline_layout_create_info{};
    pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_create_info.pushConstantRangeCount = 0;
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