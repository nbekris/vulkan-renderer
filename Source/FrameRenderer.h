#pragma once
#include <vulkan/vulkan.h>
#include <cstddef>
#include <vector>

namespace VulkanRenderer {
class VulkanContext;
class SwapChain;
class RenderTargets;
class IDrawCommands;

/** Owns frame scheduling, command buffers, and synchronization. */
class FrameRenderer {
public:
	FrameRenderer(const VulkanContext &context, const SwapChain &swapChain, const RenderTargets &targets,
				  const IDrawCommands &drawCommands);
	virtual ~FrameRenderer() noexcept;
	FrameRenderer(const FrameRenderer &) = delete;
	FrameRenderer &operator=(const FrameRenderer &) = delete;
	void Initialize();
	/** Returns false when presentation resources need to be recreated. */
	bool DrawFrame();

private:
	void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) const;
	void CreateCommandPool();
	void CreateCommandBuffers();
	void CreateSyncObjects();
	static constexpr uint32_t _MAX_FRAMES_IN_FLIGHT = 2;
	const VulkanContext &_context;
	const SwapChain &_swapChain;
	const RenderTargets &_targets;
	const IDrawCommands &_drawCommands;
	VkCommandPool _commandPool = VK_NULL_HANDLE;
	std::vector<VkCommandBuffer> _commandBuffers;
	std::vector<VkSemaphore> _imageAvailableSemaphores;
	std::vector<VkSemaphore> _renderFinishedSemaphores;
	std::vector<VkFence> _inFlightFences;
	std::size_t _currentFrame = 0;
};
} // namespace VulkanRenderer
