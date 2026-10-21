#pragma once
#include <cstdint>

enum class ResourceTypes : uint32_t{
    FrameBuffers =          1,
    ImageViews =            2,
    AllocatedImages =       3,
    AllocatedBuffer =       4,
    RenderObjects =         5,
    Samplers =              6,
    DescriptorPools =       7,
    Pipelines =             8,
    PipelineLayouts =       9,
    DescriptorSetLayouts =  10,
    RenderPasses =          11,
    CommandPools =          12,
    SwapChain =             13,
    LogicalDevice =         14,
    Surface =               15,
    DebugMessenger =        16,
    Instance =              17,
    Allocator =             18
};

enum class DescriptorUsage : uint32_t{
    BufferInfo = 0b0001,
    ImageInfo = 0b0010,
    TexelBufferView = 0b0100,
    Extension = 0b1000
};

bool operator<(ResourceTypes a, ResourceTypes b);
