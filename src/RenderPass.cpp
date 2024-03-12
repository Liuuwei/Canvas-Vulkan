#include "RenderPass.h"
#include "vulkan/vulkan_core.h"
#include <stdexcept>

RenderPass::RenderPass(VkDevice device, VkRenderPassCreateInfo renderPassInfo) : device_(device) {
    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass_) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
    }
}