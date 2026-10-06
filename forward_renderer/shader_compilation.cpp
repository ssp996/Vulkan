#include "renderer.hpp"

std::vector<char> readFile(const std::string& filepath)
{
  std::ifstream file(filepath, std::ios::ate | std::ios::binary);

    if (!file.is_open()) 
    {
        throw std::runtime_error("failed to open file!");
    }

    size_t fileSize = (size_t) file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;  
}

VkShaderModule Renderer::createShaderModule(const std::vector<char>& code, VkDevice device) 
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();

    //spir-v is built on 32 bit words, basically this line says treat this array as being filled with 32 bit integers not 1 byte chars
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) 
    {
        throw std::runtime_error("failed to create shader module!");
    }

    return shaderModule;
}