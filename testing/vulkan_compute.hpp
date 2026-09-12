#pragma once

#include "vulkan_utils_testing.hpp"

void create_compute_pipeline(const std::string& shader_filepath, VkPipelineLayout& pipeline_layout, VkPipeline& compute_pipeline, const std::vector<VkDescriptorSetLayout>& descriptor_set_layouts, VkDevice device);
