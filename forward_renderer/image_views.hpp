#pragma once

#include "include.hpp"
#include "renderer.hpp"
#include "structs.hpp"
#include "enums.hpp"

VkFormat find_supported_format(VkPhysicalDevice physical_device, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);