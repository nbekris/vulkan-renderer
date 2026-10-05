#pragma once
#include <memory>
#include <vector>
#include <cstdint>
#include <vulkan/vulkan.h>

namespace VulkanRenderer {
class VulkanContext;
class MemoryAllocator;
class FrameResources;

/** Owns a double- or triple-buffered ring without sharing command pools. */
class FrameRing {
public:
	FrameRing(const VulkanContext &context, const MemoryAllocator &allocator);
	virtual ~FrameRing();

	uint32_t GetCount() const noexcept { return static_cast<uint32_t>(_frames.size()); }

	FrameResources &GetFrame(uint32_t index) const;
	void Initialize(uint32_t frameCount, VkExtent2D depthExtent = {}, VkFormat depthFormat = VK_FORMAT_UNDEFINED,
					bool profile = false);

	FrameRing(const FrameRing &) = delete;
	FrameRing &operator=(const FrameRing &) = delete;

private:
	const VulkanContext &_context;
	const MemoryAllocator &_allocator;
	std::vector<std::unique_ptr<FrameResources>> _frames;
};
} // namespace VulkanRenderer
