#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <optional>
#include "Rendering/RenderData.h"

namespace VulkanRenderer {
class VulkanContext;
class SwapChain;
class FrameRing;
class FrameResources;
class IDrawCommands;

/** Schedules independent frame slots using Synchronization2 and dynamic rendering. */
class FrameRenderer {
public:
	FrameRenderer(const VulkanContext &context, const SwapChain &swapChain, const FrameRing &frames,
				  const IDrawCommands &drawCommands);
	virtual ~FrameRenderer() noexcept;
	FrameRenderer(const FrameRenderer &) = delete;
	FrameRenderer &operator=(const FrameRenderer &) = delete;

	std::optional<double> GetLastGpuMilliseconds() const noexcept { return _lastGpuMilliseconds; }

	void Initialize();
	/** Returns false when presentation resources need to be recreated. */
	bool DrawFrame(const FrameUniform &uniform = FrameUniform{});

private:
	void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) const;
	void TransitionImage(VkCommandBuffer commandBuffer, uint32_t imageIndex, bool beforeRendering) const;
	void SubmitFrame(FrameResources &frame, uint32_t imageIndex);
	const VulkanContext &_context;
	const SwapChain &_swapChain;
	const FrameRing &_frames;
	const IDrawCommands &_drawCommands;
	std::vector<VkSemaphore> _renderFinishedSemaphores;
	uint32_t _currentFrame = 0;
	std::optional<double> _lastGpuMilliseconds;
};
} // namespace VulkanRenderer
