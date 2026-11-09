#pragma once
#include <cstdint>

enum class ResourceTypes : uint32_t{
    Sempahores =            1,
    Fences =                2,
    FrameBuffers =          3,
    ImageViews =            4,
    AllocatedImages =       5,
    AllocatedBuffer =       6,
    RenderObjects =         7,
    Samplers =              8,
    DescriptorPools =       9,
    Pipelines =             10,
    PipelineLayouts =       11,
    DescriptorSetLayouts =  12,
    RenderPasses =          13,
    CommandPools =          14,
    SwapChain =             15,
    Allocator =             16, 
    LogicalDevice =         17, 
    Surface =               18,
    DebugMessenger =        19, 
    Instance =              20 
};

enum class DescriptorUsage : uint32_t{
    BufferInfo = 0b0001,
    ImageInfo = 0b0010,
    TexelBufferView = 0b0100,
    Extension = 0b1000
};

bool operator<(ResourceTypes a, ResourceTypes b);
