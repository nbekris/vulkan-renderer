#pragma once
#include "Rendering/Memory/VmaConfig.h"

namespace VulkanRenderer {
class MemoryAllocator;

/** Owns a device-local 2D image and its view. The caller must retire GPU use before destruction. */
class AllocatedImage {
public:
	explicit AllocatedImage(const MemoryAllocator &allocator);
	virtual ~AllocatedImage() noexcept;
	AllocatedImage(const AllocatedImage &) = delete;
	AllocatedImage &operator=(const AllocatedImage &) = delete;

	VkImage GetHandle() const noexcept { return _image; }

	VkImageView GetView() const noexcept { return _view; }

	VkExtent2D GetExtent() const noexcept { return _extent; }

	uint32_t GetMipLevels() const noexcept { return _mipLevels; }

	void Initialize(VkExtent2D extent, VkFormat format, VkImageUsageFlags usage, VkImageAspectFlags viewAspect,
					uint32_t mipLevels = 1);

private:
	const MemoryAllocator &_allocator;
	VkImage _image = VK_NULL_HANDLE;
	VmaAllocation _allocation = nullptr;
	VkImageView _view = VK_NULL_HANDLE;
	VkExtent2D _extent{};
	uint32_t _mipLevels = 0;
};
} // namespace VulkanRenderer
