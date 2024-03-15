#include "Sampler.h"
#include "vulkan/vulkan_core.h"
#include "Tools.h"

Sampler::Sampler(VkDevice device) : device_(device) {

}

void Sampler::init() {
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.flags = flags_;
    samplerInfo.magFilter = magFilter_;
    samplerInfo.minFilter = minFilter_;
    samplerInfo.mipmapMode = mipmapMode_;
    samplerInfo.addressModeU = addressModeU_;
    samplerInfo.addressModeV = addressModeV_;
    samplerInfo.addressModeW = addressModeW_;
    samplerInfo.mipLodBias = mipLodBias_;
    samplerInfo.anisotropyEnable = anisotropyEnable_;
    samplerInfo.maxAnisotropy = maxAnisotropy_;
    samplerInfo.compareEnable = compareEnable_;
    samplerInfo.compareOp = compareOp_;
    samplerInfo.minLod = minLod_;
    samplerInfo.maxLod = maxLod_;
    samplerInfo.borderColor = borderColor_;
    samplerInfo.unnormalizedCoordinates = unnormalizedCoordinates_;

    VK_CHECK(vkCreateSampler(device_, &samplerInfo, nullptr, &sampler_));
}