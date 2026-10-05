#pragma once
#include "Rendering/Memory/AllocatedImage.h"
#include "Assets/TexturePixels.h"
#include "Rendering/Resources/TextureSampling.h"

namespace VulkanRenderer {
class VulkanContext;

/** Immutable sampled RGBA8 image, mip chain, and sampler. GPU use must finish before destruction. */
class Texture {
public:
	Texture(const VulkanContext &context, const MemoryAllocator &allocator);
	virtual ~Texture() noexcept;
	Texture(const Texture &) = delete;
	Texture &operator=(const Texture &) = delete;

	VkImage GetImage() const noexcept { return _image.GetHandle(); }

	uint32_t GetMipLevels() const noexcept { return _image.GetMipLevels(); }

	VkDescriptorImageInfo GetDescriptor() const noexcept;
	void Initialize(VkExtent2D extent, std::span<const uint8_t> pixels,
					TextureColorSpace colorSpace = TextureColorSpace::Srgb, const TextureSampling &sampling = {});

private:
	const VulkanContext &_context;
	const MemoryAllocator &_allocator;
	AllocatedImage _image;
	VkSampler _sampler = VK_NULL_HANDLE;
};
} // namespace VulkanRenderer
