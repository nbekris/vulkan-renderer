#include "Rendering/Resources/Texture.h"
#include "Rendering/Memory/ResourceUploader.h"
#include "Rendering/Core/VulkanContext.h"
#include "Rendering/Core/VulkanCheck.h"

namespace VulkanRenderer {
Texture::Texture(const VulkanContext &context, const MemoryAllocator &allocator)
	: _context(context), _allocator(allocator), _image(allocator) {
}

Texture::~Texture() noexcept {
	if (_sampler) {
		vkDestroySampler(_context.GetDevice(), _sampler, nullptr);
	}
}

VkDescriptorImageInfo Texture::GetDescriptor() const noexcept {
	return {_sampler, _image.GetView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
}

void Texture::Initialize(VkExtent2D extent, std::span<const uint8_t> pixels, TextureColorSpace colorSpace,
						 const TextureSampling &sampling) {
	if (_image.GetHandle()) {
		throw std::logic_error("texture already initialized");
	}
	VkPhysicalDeviceProperties device{};
	vkGetPhysicalDeviceProperties(_context.GetPhysicalDevice(), &device);
	if (extent.width > device.limits.maxImageDimension2D || extent.height > device.limits.maxImageDimension2D) {
		throw std::invalid_argument("texture exceeds the device's 2D image size limit");
	}
	const auto FORMAT = colorSpace == TextureColorSpace::Srgb ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;
	VkFormatProperties properties{};
	vkGetPhysicalDeviceFormatProperties(_context.GetPhysicalDevice(), FORMAT, &properties);
	constexpr auto REQUIRED = VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT
							  | VK_FORMAT_FEATURE_TRANSFER_DST_BIT;
	if ((properties.optimalTilingFeatures & REQUIRED) != REQUIRED) {
		throw std::runtime_error("RGBA8 format does not support filtered sampled texture uploads");
	}
	const auto MIPS = TexturePixels::BuildMipChain(extent, pixels, colorSpace);
	_image.Initialize(extent, FORMAT, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
					  VK_IMAGE_ASPECT_COLOR_BIT, static_cast<uint32_t>(MIPS.size()));
	std::vector<uint8_t> bytes;
	std::vector<VkBufferImageCopy> regions;
	for (uint32_t i = 0; i < MIPS.size(); ++i) {
		VkBufferImageCopy region{};
		region.bufferOffset = bytes.size();
		region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, i, 0, 1};
		region.imageExtent = {MIPS[i].extent.width, MIPS[i].extent.height, 1};
		regions.push_back(region);
		bytes.insert(bytes.end(), MIPS[i].pixels.begin(), MIPS[i].pixels.end());
	}
	ResourceUploader uploader(_context, _allocator);
	uploader.Initialize();
	uploader.UploadImage(_image, bytes, regions);
	VkSamplerCreateInfo sampler{};
	sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler.magFilter = sampling.magFilter;
	sampler.minFilter = sampling.minFilter;
	sampler.mipmapMode = sampling.mipmapMode;
	sampler.addressModeU = sampling.addressU;
	sampler.addressModeV = sampling.addressV;
	sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler.maxLod = sampling.useMipmaps ? static_cast<float>(MIPS.size() - 1) : 0.0f;
	CheckVulkan(vkCreateSampler(_context.GetDevice(), &sampler, nullptr, &_sampler),
				"failed to create texture sampler");
}
} // namespace VulkanRenderer
