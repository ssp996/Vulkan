#include "reflect.hpp"

void load_reflect_module(const std::vector<char>& spv_chars, SpvReflectShaderModule& module)
{
    size_t code_size = spv_chars.size(); 

    const uint32_t* spv_code = reinterpret_cast<const uint32_t*>(spv_chars.data());
    
    SpvReflectResult result = spvReflectCreateShaderModule(code_size, spv_code, &module);

    if (result != SPV_REFLECT_RESULT_SUCCESS)
    {
        throw std::runtime_error("failed to create reflect module");
    }
}

void enumerate_descriptor_sets(SpvReflectShaderModule& module, std::vector<SpvReflectDescriptorSet*>& sets)
{
    uint32_t count = 0;
    spvReflectEnumerateDescriptorSets(&module, &count, nullptr);

    sets.resize(count);
    spvReflectEnumerateDescriptorSets(&module, &count, sets.data());
}

void map_vulkan_bindings(SpvReflectShaderModule& module, const std::vector<SpvReflectDescriptorSet*>& sets, std::vector<std::vector<VkDescriptorSetLayoutBinding>>& all_bindings, std::vector<VkDescriptorSetLayout>& layouts, VkDevice device)
{   
    std::vector<VkDescriptorSetLayoutCreateInfo> layout_infos;

    for (size_t s = 0; s < sets.size(); ++s) 
    {
        const SpvReflectDescriptorSet& refl_set = *(sets[s]);
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        for (uint32_t b = 0; b < refl_set.binding_count; ++b) 
        {
            const SpvReflectDescriptorBinding& refl_binding = *(refl_set.bindings[b]);
            
            VkDescriptorSetLayoutBinding layout_binding{};
            layout_binding.binding = refl_binding.binding;
            layout_binding.descriptorType = static_cast<VkDescriptorType>(refl_binding.descriptor_type);
            layout_binding.descriptorCount = 1; 
            
            for (uint32_t i_dim = 0; i_dim < refl_binding.array.dims_count; ++i_dim) 
            {
                layout_binding.descriptorCount *= refl_binding.array.dims[i_dim];
            }
            
            layout_binding.stageFlags = static_cast<VkShaderStageFlagBits>(module.shader_stage);
            
            bindings.push_back(layout_binding);
        }
        
        all_bindings.push_back(bindings);

        VkDescriptorSetLayoutCreateInfo layout_info{};
        layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layout_info.bindingCount = static_cast<uint32_t>(bindings.size());
        layout_info.pBindings = all_bindings.back().data();
        
        layout_infos.push_back(layout_info);
    }

    layouts.resize(layout_infos.size());

    for (int i = 0; i < layout_infos.size(); i++)
    {
        if (vkCreateDescriptorSetLayout(device, &layout_infos[i], nullptr, &layouts[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create descriptor set layout");
        }
    }
}