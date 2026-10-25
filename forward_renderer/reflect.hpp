#pragma once

#include "include.hpp"

std::vector<char> readFile(const std::string& filename);
VkShaderModule createShaderModule(const std::vector<char>& code, VkDevice device);
void load_reflect_module(const std::vector<uint32_t>& spv_chars, SpvReflectShaderModule& module);
void enumerate_descriptor_sets(SpvReflectShaderModule& module, std::vector<SpvReflectDescriptorSet*>& sets);
void map_vulkan_bindings(SpvReflectShaderModule& module, const std::vector<SpvReflectDescriptorSet*>& sets, std::vector<VkDescriptorSetLayoutCreateInfo>& layout_create_infos);
