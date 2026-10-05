#include "Rendering/Presentation/SwapChain.h"
#include "Rendering/Core/VulkanContext.h"
#include "Rendering/Core/DeviceSelection.h"
#include "Platform/Window.h"
#include "Rendering/Core/VulkanCheck.h"
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace VulkanRenderer {

SwapChain::SwapChain(const VulkanContext &context, const Window &window) : _context(context), _window(window) {
}

SwapChain::~SwapChain() noexcept {
	for (auto view : _imageViews) {
		if (view) {
			vkDestroyImageView(_context.GetDevice(), view, nullptr);
		}
	}
	if (_swapChain) {
		vkDestroySwapchainKHR(_context.GetDevice(), _swapChain, nullptr);
	}
}

void SwapChain::Initialize() {
	if (_swapChain) {
		throw std::logic_error("swapchain already initialized");
	}
	CreateSwapChain();
	CreateImageViews();
}

VkSurfaceFormatKHR SwapChain::ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &formats) {
	for (const auto &FORMAT : formats) {
		if (FORMAT.format == VK_FORMAT_B8G8R8A8_SRGB && FORMAT.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return FORMAT;
		}
	}
	return formats.front();
}

VkPresentModeKHR SwapChain::ChoosePresentMode(const std::vector<VkPresentModeKHR> &modes) {
	for (auto mode : modes) {
		if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return mode;
		}
	}
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D SwapChain::ChooseExtent(const VkSurfaceCapabilitiesKHR &capabilities) const {
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		return capabilities.currentExtent;
	}
	const auto SIZE = _window.GetFramebufferSize();
	const auto WIDTH = SIZE.width;
	const auto HEIGHT = SIZE.height;
	return {
		std::clamp(static_cast<uint32_t>(WIDTH), capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
		std::clamp(static_cast<uint32_t>(HEIGHT), capabilities.minImageExtent.height,
				   capabilities.maxImageExtent.height)};
}

void SwapChain::CreateSwapChain() {
	const auto SUPPORT = DeviceSelection::QuerySwapChainSupport(_context.GetPhysicalDevice(), _context.GetSurface());
	if (SUPPORT.formats.empty() || SUPPORT.presentModes.empty()) {
		throw std::runtime_error("surface has no supported formats or presentation modes");
	}
	const auto FORMAT = ChooseSurfaceFormat(SUPPORT.formats);
	const auto PRESENT_MODE = ChoosePresentMode(SUPPORT.presentModes);
	const auto EXTENT = ChooseExtent(SUPPORT.capabilities);
	uint32_t imageCount = SUPPORT.capabilities.minImageCount + 1;
	if (SUPPORT.capabilities.maxImageCount > 0 && imageCount > SUPPORT.capabilities.maxImageCount) {
		imageCount = SUPPORT.capabilities.maxImageCount;
	}
	VkSwapchainCreateInfoKHR info{};
	info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	info.surface = _context.GetSurface();
	info.minImageCount = imageCount;
	info.imageFormat = FORMAT.format;
	info.imageColorSpace = FORMAT.colorSpace;
	info.imageExtent = EXTENT;
	info.imageArrayLayers = 1;
	info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	QueueFamilyIndices indices;
	indices.graphicsFamily = _context.GetGraphicsFamily();
	indices.presentFamily = _context.GetPresentFamily();
	const uint32_t FAMILIES[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};
	if (indices.graphicsFamily != indices.presentFamily) {
		info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		info.queueFamilyIndexCount = 2;
		info.pQueueFamilyIndices = FAMILIES;
	} else {
		info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}
	info.preTransform = SUPPORT.capabilities.currentTransform;
	info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	info.presentMode = PRESENT_MODE;
	info.clipped = VK_TRUE;
	if (vkCreateSwapchainKHR(_context.GetDevice(), &info, nullptr, &_swapChain) != VK_SUCCESS) {
		throw std::runtime_error("failed to create swap chain");
	}
	CheckVulkan(vkGetSwapchainImagesKHR(_context.GetDevice(), _swapChain, &imageCount, nullptr),
				"failed to enumerate swapchain images");
	_images.resize(imageCount);
	CheckVulkan(vkGetSwapchainImagesKHR(_context.GetDevice(), _swapChain, &imageCount, _images.data()),
				"failed to query swapchain images");
	_imageFormat = FORMAT.format;
	_extent = EXTENT;
}

void SwapChain::CreateImageViews() {
	_imageViews.resize(_images.size());
	for (size_t i = 0; i < _images.size(); ++i) {
		VkImageViewCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		info.image = _images[i];
		info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		info.format = _imageFormat;
		info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		info.subresourceRange.levelCount = 1;
		info.subresourceRange.layerCount = 1;
		if (vkCreateImageView(_context.GetDevice(), &info, nullptr, &_imageViews[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create image view");
		}
	}
}
} // namespace VulkanRenderer
