#pragma once

#include "vulkan/vulkan_core.h"
#include <vulkan/vulkan.h>

class RenderPass {
public:
    RenderPass(VkDevice device);
    ~RenderPass();

    void init();
    VkRenderPass renderPass() const { return renderPass_; }
    void*                       pNext_{};
    VkRenderPassCreateFlags           flags_{};
    uint32_t                          attachmentCount_{};
    VkAttachmentDescription*    pAttachments_{};
    uint32_t                          subpassCount_{};
    VkSubpassDescription*       pSubpasses_{};
    uint32_t                          dependencyCount_{};
    VkSubpassDependency*        pDependencies_{};
private:
    VkDevice device_;
    VkRenderPass renderPass_;
};