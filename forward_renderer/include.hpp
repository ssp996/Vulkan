#pragma once

#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <GLFW/glfw3.h>

#include "vk_mem_alloc.h"

#include "SPIRV-Reflect/spirv_reflect.h"

#include <vector>
#include <stdexcept>
#include <cstring>
#include <iostream>
#include <optional>
#include <set>
#include <algorithm>
#include <fstream>
#include <cstddef>
#include <format>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <unordered_map>
#include <any>
#include <typeinfo>
#include <memory>
#include <map>
