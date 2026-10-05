#pragma once
#include <vulkan/vulkan.h>

namespace VulkanRenderer {
struct TextureSampling {
	VkFilter magFilter = VK_FILTER_LINEAR;
	VkFilter minFilter = VK_FILTER_LINEAR;
	VkSamplerMipmapMode mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	VkSamplerAddressMode addressU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	VkSamplerAddressMode addressV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	bool useMipmaps = true;
};
} // namespace VulkanRenderer
