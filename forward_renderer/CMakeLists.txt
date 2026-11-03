cmake_minimum_required(VERSION 3.15)
project(VulkanEngine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

# 1. Grab your engine's standard C++ files
file(GLOB SOURCES "*.cpp" "*.hpp")

# 2. Manually add the SPIRV-Reflect C implementation file
list(APPEND SOURCES "SPIRV-Reflect/spirv_reflect.c")

add_executable(${PROJECT_NAME} ${SOURCES})

# 3. Add the include directories so the compiler can find the headers
target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    
    # Allows #include "spirv_reflect.h"
    ${CMAKE_CURRENT_SOURCE_DIR}/SPIRV-Reflect 
    
    # Allows spirv_reflect.h to find "spirv/unified1/spirv.h" internally
    ${CMAKE_CURRENT_SOURCE_DIR}/SPIRV-Reflect/include 
    
    # NOTE: If your vk_mem_alloc.h is inside a specific folder (like "include"), add it here:
    # ${CMAKE_CURRENT_SOURCE_DIR}/include 
)

target_link_libraries(${PROJECT_NAME} PRIVATE 
    Vulkan::Vulkan 
    glfw
    glm::glm
)