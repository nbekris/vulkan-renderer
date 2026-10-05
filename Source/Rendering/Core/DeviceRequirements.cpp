#include "Rendering/Core/DeviceRequirements.h"
#include "Rendering/RenderData.h"

namespace VulkanRenderer {

void DeviceRequirements::Enable(VkPhysicalDeviceFeatures2 &features, VkPhysicalDeviceVulkan12Features &features12,
								VkPhysicalDeviceVulkan13Features &features13) {
	features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	features.pNext = &features12;
	features.features.shaderUniformBufferArrayDynamicIndexing = VK_TRUE;
	features.features.shaderStorageBufferArrayDynamicIndexing = VK_TRUE;
	features.features.shaderSampledImageArrayDynamicIndexing = VK_TRUE;
	features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
	features12.pNext = &features13;
	features12.bufferDeviceAddress = VK_TRUE;
	features12.descriptorBindingPartiallyBound = VK_TRUE;
	features12.runtimeDescriptorArray = VK_TRUE;
	features12.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
	features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
	features13.synchronization2 = VK_TRUE;
	features13.dynamicRendering = VK_TRUE;
	features13.shaderDemoteToHelperInvocation = VK_TRUE;
}

bool DeviceRequirements::IsSupported(VkPhysicalDevice device) {
	VkPhysicalDeviceProperties properties{};
	vkGetPhysicalDeviceProperties(device, &properties);
	if (properties.apiVersion < VK_API_VERSION_1_3
		|| properties.limits.maxPerStageDescriptorUniformBuffers < MAX_FRAMES_IN_FLIGHT
		|| properties.limits.maxDescriptorSetUniformBuffers < MAX_FRAMES_IN_FLIGHT
		|| properties.limits.maxPerStageDescriptorStorageBuffers < MATERIAL_CAPACITY
		|| properties.limits.maxDescriptorSetStorageBuffers < MATERIAL_CAPACITY
		|| properties.limits.maxPerStageDescriptorSampledImages < TEXTURE_CAPACITY
		|| properties.limits.maxDescriptorSetSampledImages < TEXTURE_CAPACITY
		|| properties.limits.maxPerStageDescriptorSamplers < TEXTURE_CAPACITY
		|| properties.limits.maxDescriptorSetSamplers < TEXTURE_CAPACITY
		|| properties.limits.maxPerStageResources < MAX_FRAMES_IN_FLIGHT + MATERIAL_CAPACITY + TEXTURE_CAPACITY) {
		return false;
	}
	VkPhysicalDeviceFeatures2 features{};
	VkPhysicalDeviceVulkan12Features features12{};
	VkPhysicalDeviceVulkan13Features features13{};
	Enable(features, features12, features13);
	vkGetPhysicalDeviceFeatures2(device, &features);
	return features.features.shaderUniformBufferArrayDynamicIndexing
		   && features.features.shaderStorageBufferArrayDynamicIndexing
		   && features.features.shaderSampledImageArrayDynamicIndexing && features12.bufferDeviceAddress
		   && features12.shaderSampledImageArrayNonUniformIndexing && features12.descriptorBindingPartiallyBound
		   && features12.runtimeDescriptorArray && features13.synchronization2 && features13.dynamicRendering
		   && features13.shaderDemoteToHelperInvocation;
}

} // namespace VulkanRenderer
