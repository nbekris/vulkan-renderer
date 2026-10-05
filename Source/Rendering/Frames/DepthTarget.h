#pragma once
#include "Rendering/Memory/AllocatedImage.h"

namespace VulkanRenderer {
class VulkanContext;

/** One frame slot's depth attachment and Synchronization2 layout transition. */
class DepthTarget {
public:
	static VkFormat SelectFormat(const VulkanContext &context);
	explicit DepthTarget(const MemoryAllocator &allocator);
	virtual ~DepthTarget() = default;
	DepthTarget(const DepthTarget &) = delete;
	DepthTarget &operator=(const DepthTarget &) = delete;

	VkFormat GetFormat() const noexcept { return _format; }

	VkImage GetImage() const noexcept { return _image.GetHandle(); }

	VkExtent2D GetExtent() const noexcept { return _image.GetExtent(); }

	VkRenderingAttachmentInfo GetAttachment() const;
	void Initialize(VkExtent2D extent, VkFormat format);
	/** Call only after this slot's fence has completed. Clears discard the previous depth contents. */
	void Prepare(VkCommandBuffer commandBuffer) const;

private:
	AllocatedImage _image;
	VkFormat _format = VK_FORMAT_UNDEFINED;
};
} // namespace VulkanRenderer
