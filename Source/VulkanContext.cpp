#include "VulkanContext.h"
#include "Window.h"
#include "DeviceSelection.h"
#include "VulkanCheck.h"
#include <GLFW/glfw3.h>
#include <set>
#include <vector>

namespace VulkanRenderer {

VulkanContext::VulkanContext(const Window &window) : _window(window) {
}

VulkanContext::~VulkanContext() noexcept {
	if (_device) {
		vkDeviceWaitIdle(_device);
		vkDestroyDevice(_device, nullptr);
	}
	if (_surface) {
		vkDestroySurfaceKHR(_instance, _surface, nullptr);
	}
	if (_instance) {
		vkDestroyInstance(_instance, nullptr);
	}
}

void VulkanContext::Initialize() {
	if (_instance) {
		throw std::logic_error("Vulkan context already initialized");
	}
	CreateInstance();
	CreateSurface();
	PickPhysicalDevice();
	CreateLogicalDevice();
}

void VulkanContext::WaitIdle() const {
	CheckVulkan(vkDeviceWaitIdle(_device), "failed to wait for device");
}

void VulkanContext::CreateInstance() {
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Vulkan Triangle";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;
	uint32_t extensionCount = 0;
	const char **extensions = glfwGetRequiredInstanceExtensions(&extensionCount);
	if (!extensions) {
		throw std::runtime_error("failed to get GLFW Vulkan extensions");
	}
	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = extensionCount;
	createInfo.ppEnabledExtensionNames = extensions;
	if (vkCreateInstance(&createInfo, nullptr, &_instance) != VK_SUCCESS) {
		throw std::runtime_error("failed to create Vulkan instance");
	}
}

void VulkanContext::CreateSurface() {
	if (glfwCreateWindowSurface(_instance, _window.GetHandle(), nullptr, &_surface) != VK_SUCCESS) {
		throw std::runtime_error("failed to create window surface");
	}
}

void VulkanContext::PickPhysicalDevice() {
	uint32_t count = 0;
	CheckVulkan(vkEnumeratePhysicalDevices(_instance, &count, nullptr), "failed to enumerate physical devices");
	if (!count) {
		throw std::runtime_error("failed to find a Vulkan-capable GPU");
	}
	std::vector<VkPhysicalDevice> devices(count);
	CheckVulkan(vkEnumeratePhysicalDevices(_instance, &count, devices.data()), "failed to query physical devices");
	for (auto candidate : devices) {
		if (DeviceSelection::IsSuitable(candidate, _surface)) {
			_physicalDevice = candidate;
			break;
		}
	}
	if (!_physicalDevice) {
		throw std::runtime_error("failed to find a GPU with swap-chain support");
	}
}

void VulkanContext::CreateLogicalDevice() {
	auto indices = DeviceSelection::FindQueueFamilies(_physicalDevice, _surface);
	_graphicsFamily = indices.graphicsFamily.value();
	_presentFamily = indices.presentFamily.value();
	std::set<uint32_t> uniqueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};
	float priority = 1.0f;
	std::vector<VkDeviceQueueCreateInfo> queueInfos;
	for (uint32_t family : uniqueFamilies) {
		VkDeviceQueueCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		info.queueFamilyIndex = family;
		info.queueCount = 1;
		info.pQueuePriorities = &priority;
		queueInfos.push_back(info);
	}
	VkPhysicalDeviceFeatures features{};
	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueInfos.size());
	createInfo.pQueueCreateInfos = queueInfos.data();
	createInfo.pEnabledFeatures = &features;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(DEVICE_EXTENSIONS.size());
	createInfo.ppEnabledExtensionNames = DEVICE_EXTENSIONS.data();
	if (vkCreateDevice(_physicalDevice, &createInfo, nullptr, &_device) != VK_SUCCESS) {
		throw std::runtime_error("failed to create logical device");
	}
	vkGetDeviceQueue(_device, indices.graphicsFamily.value(), 0, &_graphicsQueue);
	vkGetDeviceQueue(_device, indices.presentFamily.value(), 0, &_presentQueue);
}
} // namespace VulkanRenderer
