#pragma once
#include <vulkan/vulkan.h>
#include <vector>

namespace VulkanRenderer {
class VulkanContext;
class Window;

/** Owns the swapchain and its image views; images are owned by Vulkan. */
class SwapChain {
public:
	SwapChain(const VulkanContext &context, const Window &window);
	virtual ~SwapChain() noexcept;
	SwapChain(const SwapChain &) = delete;
	SwapChain &operator=(const SwapChain &) = delete;

	VkSwapchainKHR GetHandle() const noexcept { return _swapChain; }

	VkFormat GetImageFormat() const noexcept { return _imageFormat; }

	VkExtent2D GetExtent() const noexcept { return _extent; }

	const std::vector<VkImageView> &GetImageViews() const noexcept { return _imageViews; }

	VkImage GetImage(uint32_t imageIndex) const { return _images.at(imageIndex); }

	void Initialize();

private:
	static VkSurfaceFormatKHR ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &formats);
	static VkPresentModeKHR ChoosePresentMode(const std::vector<VkPresentModeKHR> &modes);
	VkExtent2D ChooseExtent(const VkSurfaceCapabilitiesKHR &capabilities) const;
	void CreateSwapChain();
	void CreateImageViews();
	const VulkanContext &_context;
	const Window &_window;
	VkSwapchainKHR _swapChain = VK_NULL_HANDLE;
	VkFormat _imageFormat = VK_FORMAT_UNDEFINED;
	VkExtent2D _extent{};
	std::vector<VkImage> _images;
	std::vector<VkImageView> _imageViews;
};
} // namespace VulkanRenderer
