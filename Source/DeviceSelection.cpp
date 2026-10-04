#include "DeviceSelection.h"
#include "VulkanCheck.h"
#include <set>
#include <string>

namespace VulkanRenderer {
QueueFamilyIndices DeviceSelection::FindQueueFamilies(VkPhysicalDevice candidate, VkSurfaceKHR surface) {
	QueueFamilyIndices indices;
	uint32_t count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(candidate, &count, nullptr);
	std::vector<VkQueueFamilyProperties> families(count);
	vkGetPhysicalDeviceQueueFamilyProperties(candidate, &count, families.data());
	for (uint32_t i = 0; i < count; ++i) {
		if (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphicsFamily = i;
		}
		VkBool32 presentSupport = VK_FALSE;
		CheckVulkan(vkGetPhysicalDeviceSurfaceSupportKHR(candidate, i, surface, &presentSupport),
					"failed to query presentation support");
		if (presentSupport) {
			indices.presentFamily = i;
		}
		if (indices.IsComplete()) {
			break;
		}
	}
	return indices;
}

SwapChainSupportDetails DeviceSelection::QuerySwapChainSupport(VkPhysicalDevice candidate, VkSurfaceKHR surface) {
	SwapChainSupportDetails details;
	CheckVulkan(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(candidate, surface, &details.capabilities),
				"failed to query surface capabilities");
	uint32_t count = 0;
	CheckVulkan(vkGetPhysicalDeviceSurfaceFormatsKHR(candidate, surface, &count, nullptr),
				"failed to enumerate surface formats");
	if (count) {
		details.formats.resize(count);
		CheckVulkan(vkGetPhysicalDeviceSurfaceFormatsKHR(candidate, surface, &count, details.formats.data()),
					"failed to query surface formats");
	}
	CheckVulkan(vkGetPhysicalDeviceSurfacePresentModesKHR(candidate, surface, &count, nullptr),
				"failed to enumerate presentation modes");
	if (count) {
		details.presentModes.resize(count);
		CheckVulkan(vkGetPhysicalDeviceSurfacePresentModesKHR(candidate, surface, &count, details.presentModes.data()),
					"failed to query presentation modes");
	}
	return details;
}

bool DeviceSelection::CheckDeviceExtensionSupport(VkPhysicalDevice candidate) {
	uint32_t count = 0;
	CheckVulkan(vkEnumerateDeviceExtensionProperties(candidate, nullptr, &count, nullptr),
				"failed to enumerate device extensions");
	std::vector<VkExtensionProperties> extensions(count);
	CheckVulkan(vkEnumerateDeviceExtensionProperties(candidate, nullptr, &count, extensions.data()),
				"failed to query device extensions");
	std::set<std::string> required(DEVICE_EXTENSIONS.begin(), DEVICE_EXTENSIONS.end());
	for (const auto &EXTENSION : extensions) {
		required.erase(EXTENSION.extensionName);
	}
	return required.empty();
}

bool DeviceSelection::IsSuitable(VkPhysicalDevice candidate, VkSurfaceKHR surface) {
	const bool EXTENSIONS_SUPPORTED = CheckDeviceExtensionSupport(candidate);
	bool swapChainAdequate = false;
	if (EXTENSIONS_SUPPORTED) {
		auto support = QuerySwapChainSupport(candidate, surface);
		swapChainAdequate = !support.formats.empty() && !support.presentModes.empty();
	}
	return FindQueueFamilies(candidate, surface).IsComplete() && EXTENSIONS_SUPPORTED && swapChainAdequate;
}
} // namespace VulkanRenderer
