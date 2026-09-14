#pragma once

#include "vulkan_utils_testing.hpp"

void create_compute_pipeline(const std::string& shader_filepath, VkPipelineLayout& pipeline_layout, VkPipeline& compute_pipeline, const std::vector<VkDescriptorSetLayout>& descriptor_set_layouts, VkDevice device, const std::vector<VkPushConstantRange>& push_constant_ranges);

int is_compute_ready(VkPhysicalDevice physical_device);

void create_compute_device(VkPhysicalDevice physical_device, VkDevice& device, VkQueue& compute_queue);

void pick_compute_physical_device(VkInstance instance, VkPhysicalDevice& physical_device);

