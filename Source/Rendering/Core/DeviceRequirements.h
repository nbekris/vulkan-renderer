#pragma once
#include <vulkan/vulkan.h>

namespace VulkanRenderer {
/** Centralizes the Vulkan 1.3 rendering and descriptor feature contract. */
class DeviceRequirements {
public:
	static bool IsSupported(VkPhysicalDevice device);
	static void Enable(VkPhysicalDeviceFeatures2 &features, VkPhysicalDeviceVulkan12Features &features12,
					   VkPhysicalDeviceVulkan13Features &features13);
	DeviceRequirements() = delete;
	virtual ~DeviceRequirements() = default;

	DeviceRequirements(const DeviceRequirements &) = delete;
	DeviceRequirements &operator=(const DeviceRequirements &) = delete;

private:
};
} // namespace VulkanRenderer
