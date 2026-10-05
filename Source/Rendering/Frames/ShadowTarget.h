#pragma once
#include "Rendering/Memory/AllocatedImage.h"

namespace VulkanRenderer {
class VulkanContext;

/** Per-frame sampled depth image; frame fences retire all access before reuse. */
class ShadowTarget {
public:
	static constexpr uint32_t RESOLUTION = 2048;
	ShadowTarget(const VulkanContext &context, const MemoryAllocator &allocator);
	virtual ~ShadowTarget() noexcept;
	ShadowTarget(const ShadowTarget &) = delete;
	ShadowTarget &operator=(const ShadowTarget &) = delete;

	VkFormat GetFormat() const noexcept { return _format; }

	VkDescriptorImageInfo GetDescriptor() const;
	VkRenderingAttachmentInfo GetAttachment() const;
	void Initialize();
	void Transition(VkCommandBuffer command, bool forRendering) const;

private:
	const VulkanContext &_context;
	AllocatedImage _image;
	VkSampler _sampler = VK_NULL_HANDLE;
	VkFormat _format = VK_FORMAT_UNDEFINED;
};
} // namespace VulkanRenderer
