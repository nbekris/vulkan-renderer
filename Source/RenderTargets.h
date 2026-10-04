#pragma once
#include <vulkan/vulkan.h>
#include <vector>

namespace VulkanRenderer {
class VulkanContext;
class SwapChain;

/** Owns the color render pass and framebuffers. */
class RenderTargets {
public:
	RenderTargets(const VulkanContext &context, const SwapChain &swapChain);
	virtual ~RenderTargets() noexcept;
	RenderTargets(const RenderTargets &) = delete;
	RenderTargets &operator=(const RenderTargets &) = delete;

	VkRenderPass GetRenderPass() const noexcept { return _renderPass; }

	VkFramebuffer GetFramebuffer(uint32_t imageIndex) const { return _framebuffers.at(imageIndex); }

	void Initialize();

private:
	void CreateRenderPass();
	void CreateFramebuffers();
	const VulkanContext &_context;
	const SwapChain &_swapChain;
	VkRenderPass _renderPass = VK_NULL_HANDLE;
	std::vector<VkFramebuffer> _framebuffers;
};
} // namespace VulkanRenderer
