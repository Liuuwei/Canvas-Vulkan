#pragma once

#include "vulkan/vulkan_core.h"
#include <vulkan/vulkan.h>

class Sampler {
public:
    Sampler(VkDevice device);

    void init();
    VkSampler sampler() const { return sampler_; }

public:
    VkSamplerCreateFlags flags_ = 0;
    VkFilter magFilter_ = VK_FILTER_LINEAR;
    VkFilter minFilter_ = VK_FILTER_LINEAR;
    VkSamplerMipmapMode mipmapMode_ = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    VkSamplerAddressMode addressModeU_ = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    VkSamplerAddressMode addressModeV_ = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    VkSamplerAddressMode addressModeW_ = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    float mipLodBias_{};
    VkBool32 anisotropyEnable_{};
    float maxAnisotropy_{};
    VkBool32 compareEnable_{};
    VkCompareOp compareOp_{};
    float minLod_{};
    float maxLod_{};
    VkBorderColor borderColor_{};
    VkBool32 unnormalizedCoordinates_{};

private:
    VkDevice device_;
    VkSampler sampler_;
};