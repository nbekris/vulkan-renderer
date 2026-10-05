#pragma once
#include <vulkan/vulkan.h>
#include <array>
#include <optional>
#include <vector>

namespace VulkanRenderer {
inline constexpr std::array<const char *, 1> DEVICE_EXTENSIONS = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	bool IsComplete() const { return graphicsFamily.has_value() && presentFamily.has_value(); }
};

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities{};
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

/** Queries device capabilities without owning Vulkan resources. */
class DeviceSelection {
public:
	static QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice candidate, VkSurfaceKHR surface);
	static SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice candidate, VkSurfaceKHR surface);
	static bool IsSuitable(VkPhysicalDevice candidate, VkSurfaceKHR surface);
	DeviceSelection() = delete;
	DeviceSelection(const DeviceSelection &) = delete;
	DeviceSelection &operator=(const DeviceSelection &) = delete;
	virtual ~DeviceSelection() = default;

private:
	static bool CheckDeviceExtensionSupport(VkPhysicalDevice candidate);
};
} // namespace VulkanRenderer
