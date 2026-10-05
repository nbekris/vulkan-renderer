#pragma once
#include <vulkan/vulkan.h>
#include <stdexcept>
#include <string>

namespace VulkanRenderer {
inline void CheckVulkan(VkResult result, const char *operation) {
	if (result != VK_SUCCESS) {
		throw std::runtime_error(std::string(operation) + " (VkResult " + std::to_string(result) + ")");
	}
}
} // namespace VulkanRenderer
