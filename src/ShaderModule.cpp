#include "ShaderModule.h"

#include "VulkanHelp.h"
#include "vulkan/vulkan_core.h"
#include <cstdint>
#include <stdexcept>

ShaderModule::ShaderModule(VkDevice device, const std::string& path) : device_(device) {
    auto code = VulkanHelp::readFile(path);

    VkShaderModuleCreateInfo shaderInfo{};
    shaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderInfo.codeSize = code.size();
    shaderInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

    if (vkCreateShaderModule(device, &shaderInfo, nullptr, &shader_) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader!");
    }
}