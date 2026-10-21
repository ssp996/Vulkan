#include "renderer.hpp"

void Renderer::create_descriptor_pool()
{   
    std::array<VkDescriptorPoolSize, 3> pool_sizes{};

    pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pool_sizes[0].descriptorCount = MAX_DESCRIPTOR_SETS * 1;

    pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    pool_sizes[1].descriptorCount = MAX_DESCRIPTOR_SETS * 4;

    pool_sizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    pool_sizes[2].descriptorCount = MAX_DESCRIPTOR_SETS * 1;

    VkDescriptorPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
    pool_info.pPoolSizes = pool_sizes.data();
    pool_info.maxSets = MAX_DESCRIPTOR_SETS;

    if (vkCreateDescriptorPool(device, &pool_info, nullptr, &descriptor_pool) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create descriptor pool");
    }
}


void create_custom_descriptor_set_layout(VkDevice device, VkDescriptorSetLayout& descriptor_set_layout, const std::vector<uint32_t>& bindings, const std::vector<uint32_t>& descriptor_counts, const std::vector<VkDescriptorType>& descriptor_types, const std::vector<VkSampler*> samplers, const std::vector<VkShaderStageFlags>& stage_flags)
{
    std::vector<VkDescriptorSetLayoutBinding> layout_bindings;
    for (int i = 0; i < bindings.size(); i++)
    {
        VkDescriptorSetLayoutBinding curr_layout{};
        curr_layout.binding = bindings[i];
        curr_layout.descriptorCount = descriptor_counts[i];
        curr_layout.descriptorType = descriptor_types[i];
        curr_layout.stageFlags = stage_flags[i];
        curr_layout.pImmutableSamplers = samplers[i];

        layout_bindings.push_back(curr_layout);
    }

    VkDescriptorSetLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = static_cast<uint32_t>(layout_bindings.size());
    layout_info.pBindings = layout_bindings.data();

    if (vkCreateDescriptorSetLayout(device, &layout_info, nullptr, &descriptor_set_layout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create descriptor set layout");
    }
}

DescriptorSet::DescriptorSet(uint32_t s) : set_number(s) {};

void DescriptorSet::add_descrpitor_info(DescriptorInfo info)
{
    descriptor_infos.push_back(info);
}

void DescriptorSet::create_set_layout(VkDevice device)
{
    VkDescriptorSetLayoutCreateInfo layout_info{};

    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

    std::vector<VkDescriptorSetLayoutBinding> layout_bindings;
    for (auto info: descriptor_infos)
    {   
        layout_bindings.push_back(info.layout_binding);
    }

    layout_info.bindingCount = static_cast<uint32_t>(layout_bindings.size());
    layout_info.pBindings = layout_bindings.data();

    if (vkCreateDescriptorSetLayout(device, &layout_info, nullptr, &layout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create descriptor set layout");
    }
}

void DescriptorSet::create_descriptor_set(uint32_t n, VkDescriptorPool descriptor_pool, VkDevice device)
{
    std::vector<VkDescriptorSetLayout> layouts(n, layout);

    VkDescriptorSetAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = descriptor_pool; 
    alloc_info.descriptorSetCount = n;          
    alloc_info.pSetLayouts = layouts.data();

    descriptor_sets.resize(n);

    if (vkAllocateDescriptorSets(device, &alloc_info, descriptor_sets.data()) != VK_SUCCESS) 
    {
        throw std::runtime_error("Failed to allocate main pass descriptor sets!");
    }
    
    std::vector<VkWriteDescriptorSet> descriptor_writes;
    
    for (uint32_t i = 0; i < n; i++)
    {
        for (const auto& info : descriptor_infos)
        {
            VkWriteDescriptorSet write{};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            
            write.dstSet = descriptor_sets[i]; 
            
            write.dstBinding = info.layout_binding.binding;
            write.dstArrayElement = 0;
            write.descriptorType = info.descriptor_type;
            write.descriptorCount = info.layout_binding.descriptorCount;

            uint32_t offset = i * info.layout_binding.descriptorCount;

            // 2. Safely grab the pointer to the correct element for this frame
            bool has_image  = static_cast<uint32_t>(info.usage_flag) & static_cast<uint32_t>(DescriptorUsage::ImageInfo);
            bool has_buffer = static_cast<uint32_t>(info.usage_flag) & static_cast<uint32_t>(DescriptorUsage::BufferInfo);
            bool has_texel  = static_cast<uint32_t>(info.usage_flag) & static_cast<uint32_t>(DescriptorUsage::TexelBufferView);
            bool has_ext    = static_cast<uint32_t>(info.usage_flag) & static_cast<uint32_t>(DescriptorUsage::Extension);

            write.pImageInfo = has_image  ? &info.image_info[offset] : nullptr;
            write.pBufferInfo = has_buffer ? &info.buffer_info[offset] : nullptr;
            write.pTexelBufferView = has_texel ? &info.texel_buffer_view[offset] : nullptr;
            
            write.pNext = has_ext ? info.extension : nullptr;

            descriptor_writes.push_back(write);
        }
    }

    vkUpdateDescriptorSets(device, descriptor_writes.size(), descriptor_writes.data(), 0, nullptr);
}