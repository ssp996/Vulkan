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

    resources.push_back(ResourceTypes::DescriptorPools, descriptor_pool);
    
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

/* void DescriptorSet::create_descriptor_set(uint32_t n, VkDescriptorPool descriptor_pool, VkDevice device)
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

            bool has_image = static_cast<uint32_t>(info.usage_flag) & static_cast<uint32_t>(DescriptorUsage::ImageInfo);
            bool has_buffer = static_cast<uint32_t>(info.usage_flag) & static_cast<uint32_t>(DescriptorUsage::BufferInfo);
            bool has_texel = static_cast<uint32_t>(info.usage_flag) & static_cast<uint32_t>(DescriptorUsage::TexelBufferView);
            bool has_ext = static_cast<uint32_t>(info.usage_flag) & static_cast<uint32_t>(DescriptorUsage::Extension);

            write.pImageInfo = has_image  ? &info.image_info[offset] : nullptr;
            write.pBufferInfo = has_buffer ? &info.buffer_info[offset] : nullptr;
            write.pTexelBufferView = has_texel ? &info.texel_buffer_view[offset] : nullptr;
            
            write.pNext = has_ext ? info.extension : nullptr;

            descriptor_writes.push_back(write);
        }
    }

    vkUpdateDescriptorSets(device, descriptor_writes.size(), descriptor_writes.data(), 0, nullptr);
} */

void Renderer::allocate_descriptor_set(VkDescriptorSetLayout layout, std::vector<VkDescriptorSet>& descriptor_sets, uint32_t n)
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
        throw std::runtime_error("failed to allocate descriptor sets");
    }
}

/* void Renderer::create_descriptor_sets(std::vector<uint32_t> set_sizes) //same number of sizes as number of descriptor sets
{
    descriptor_sets.resize(reflect_descriptor_sets.size());

    for (int i = 0; i < reflect_descriptor_sets.size(); i++)
    {
        std::vector<VkDescriptorSetLayoutBinding> curr_binding = all_bindings[i];
        std::vector<VkDescriptorSet> curr_set = descriptor_sets[i];
        std::vector<VkWriteDescriptorSet> writes;

        allocate_descriptor_set(descriptor_set_layouts[i], curr_set, set_sizes[i]);

        for (int j = 0; j < curr_binding.size(); j++)
        {
            for (int k = 0; k < set_sizes[i]; k++)
            {
                 VkWriteDescriptorSet write{};
                write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                write.dstSet = curr_set[k];
                write.dstBinding = curr_binding[j].binding;
                write.dstArrayElement = 0;
                write.descriptorCount = curr_binding[j].descriptorCount;

                write.descriptorType = static_cast<VkDescriptorType>(curr_binding[j].descriptorType);

                
            }
        }
    }
} */

std::map<uint32_t, SetLayoutInfo> Renderer::build_set_layouts()
{
    std::map<uint32_t, SetLayoutInfo> out;

    for (const auto* module : reflect_shader_modules)
    {
        uint32_t n_sets = 0;

        spvReflectEnumerateDescriptorSets(module, &n_sets, nullptr);
        std::vector<SpvReflectDescriptorSet*> reflect_sets(n_sets);
        spvReflectEnumerateDescriptorSets(module, &n_sets, reflect_sets.data());

        for (const auto* reflect_set : reflect_sets)
        {
            SetLayoutInfo& info = out[reflect_set->set];
            info.set = reflect_set->set;

            for (uint32_t binding = 0; binding < reflect_set->binding_count; binding++)
            {
                const auto& reflect_binding = *reflect_set->bindings[binding];
                auto shader_stage = static_cast<VkShaderStageFlags>(module->shader_stage);
                auto is_binding = std::find_if(info.bindings.begin(), info.bindings.end(), [&](auto& x){return x.binding == reflect_binding.binding;});

                if (is_binding != info.bindings.end())
                {
                    is_binding->stageFlags |= shader_stage;
                    continue;
                }

                uint32_t count = 1;

                for (uint32_t dimension = 0; dimension < reflect_binding.array.dims_count; dimension++)
                {
                    count *= reflect_binding.array.dims[dimension];
                }

                info.bindings.push_back({
                        reflect_binding.binding,
                        static_cast<VkDescriptorType>(reflect_binding.descriptor_type),
                        count,
                        shader_stage,
                        nullptr
                    });

                if (reflect_binding.name)
                {
                    info.names[reflect_binding.name] = reflect_binding.binding;
                }
            }
        }
    }

    for (auto& [idx, info] : out)
    {
        VkDescriptorSetLayoutCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        create_info.bindingCount = static_cast<uint32_t>(info.bindings.size());
        create_info.pBindings = info.bindings.data();

        if (vkCreateDescriptorSetLayout(device, &create_info, nullptr, &info.layout) != VK_SUCCESS)
        {
            throw std::runtime_error(std::format("failed to create descriptor set layout: {}", info.set));
        }
    }

    return out;
}

void Renderer::write_set(VkDescriptorSet set, const SetLayoutInfo& set_layout_info, const std::vector<BindingData>& binding_data)
{
    std::vector<VkWriteDescriptorSet> writes;
    writes.reserve(binding_data.size());

    for (const auto& data: binding_data)
    {
        const auto& layout_binding = set_layout_info.get_set_layout_binding(data.name);
        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = set;
        write.dstBinding =  layout_binding.binding;
        write.dstArrayElement = 0;
        write.descriptorType = layout_binding.descriptorType;
        write.descriptorCount = layout_binding.descriptorCount;
        write.pBufferInfo = data.buffers.empty() ? nullptr : data.buffers.data();
        write.pImageInfo = data.images.empty() ? nullptr : data.images.data();

        writes.push_back(write);
    }

    vkUpdateDescriptorSets(device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
}

VkDescriptorBufferInfo Renderer::make_buffer_info(const AllocatedBuffer& allocated_buffer, uint32_t frame)
{
    VkDescriptorBufferInfo buf_info{};
    buf_info.buffer = allocated_buffer.buffer;
    buf_info.offset = frame * allocated_buffer.padded_size;
    buf_info.range = allocated_buffer.elem_size;

    return buf_info;
}

VkDescriptorImageInfo Renderer::make_image_info(const VkImageView& image_view, VkSampler sampler, VkImageLayout image_layout)
{
    VkDescriptorImageInfo img_info{};
    img_info.imageLayout = image_layout;
    img_info.imageView = image_view;
    img_info.sampler = sampler;
        
    return img_info;
}
