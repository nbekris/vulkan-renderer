#pragma once
#include "Rendering/Memory/AllocatedBuffer.h"
#include "Rendering/RenderData.h"
#include "Rendering/Frames/DepthTarget.h"
#include "Diagnostics/GpuTiming.h"

namespace VulkanRenderer {
class VulkanContext;

/** One CPU/GPU frame slot with independent commands, uniform data, and pacing. */
class FrameResources {
public:
	FrameResources(const VulkanContext &context, const MemoryAllocator &allocator);
	virtual ~FrameResources() noexcept;

	VkCommandBuffer GetCommandBuffer() const noexcept { return _commandBuffer; }

	VkSemaphore GetImageAvailable() const noexcept { return _imageAvailable; }

	VkFence GetFence() const noexcept { return _fence; }

	const AllocatedBuffer &GetUniformBuffer() const noexcept { return _uniformBuffer; }

	const DepthTarget &GetDepthTarget() const noexcept { return _depthTarget; }

	GpuTiming &GetTiming() noexcept { return _timing; }

	void Initialize(VkExtent2D depthExtent = {}, VkFormat depthFormat = VK_FORMAT_UNDEFINED, bool profile = false);
	void Wait();
	void Prepare(const FrameUniform &uniform);

	void MarkSubmitted() noexcept { _pending = true; }

	FrameResources(const FrameResources &) = delete;
	FrameResources &operator=(const FrameResources &) = delete;

private:
	const VulkanContext &_context;
	AllocatedBuffer _uniformBuffer;
	DepthTarget _depthTarget;
	GpuTiming _timing;
	VkCommandPool _commandPool = VK_NULL_HANDLE;
	VkCommandBuffer _commandBuffer = VK_NULL_HANDLE;
	VkSemaphore _imageAvailable = VK_NULL_HANDLE;
	VkFence _fence = VK_NULL_HANDLE;
	bool _pending = false;
};
} // namespace VulkanRenderer
