#include "RenderPass.h"
#include "vulkan/vulkan_core.h"
#include "Tools.h"

RenderPass::RenderPass(VkDevice device) : device_(device) {

}

RenderPass::~RenderPass() {
    vkDestroyRenderPass(device_, renderPass_, nullptr);
}

void RenderPass::init() {
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.flags = flags_;
    renderPassInfo.pNext = pNext_;
    renderPassInfo.subpassCount = subpassCount_;
    renderPassInfo.pSubpasses = pSubpasses_;
    renderPassInfo.attachmentCount = attachmentCount_;
    renderPassInfo.pAttachments = pAttachments_;
    renderPassInfo.dependencyCount = dependencyCount_;
    renderPassInfo.pDependencies = pDependencies_;
    VK_CHECK(vkCreateRenderPass(device_, &renderPassInfo, nullptr, &renderPass_));
}