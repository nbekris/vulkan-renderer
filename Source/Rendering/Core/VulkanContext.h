#pragma once
#include <vulkan/vulkan.h>

namespace VulkanRenderer {
class Window;

/** Owns the instance, presentation surface, logical device, and borrowed queues. */
class VulkanContext {
public:
	explicit VulkanContext(const Window &window);
	virtual ~VulkanContext() noexcept;
	VulkanContext(const VulkanContext &) = delete;
	VulkanContext &operator=(const VulkanContext &) = delete;

	VkInstance GetInstance() const noexcept { return _instance; }

	VkSurfaceKHR GetSurface() const noexcept { return _surface; }

	VkPhysicalDevice GetPhysicalDevice() const noexcept { return _physicalDevice; }

	VkDevice GetDevice() const noexcept { return _device; }

	VkQueue GetGraphicsQueue() const noexcept { return _graphicsQueue; }

	VkQueue GetPresentQueue() const noexcept { return _presentQueue; }

	uint32_t GetGraphicsFamily() const noexcept { return _graphicsFamily; }

	uint32_t GetPresentFamily() const noexcept { return _presentFamily; }

	void WaitIdle() const;
	void Initialize();

private:
	void CreateInstance();
	void CreateSurface();
	void PickPhysicalDevice();
	void CreateLogicalDevice();
	const Window &_window;
	VkInstance _instance = VK_NULL_HANDLE;
	VkSurfaceKHR _surface = VK_NULL_HANDLE;
	VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;
	VkDevice _device = VK_NULL_HANDLE;
	VkQueue _graphicsQueue = VK_NULL_HANDLE;
	VkQueue _presentQueue = VK_NULL_HANDLE;
	uint32_t _graphicsFamily = 0;
	uint32_t _presentFamily = 0;
};
} // namespace VulkanRenderer
