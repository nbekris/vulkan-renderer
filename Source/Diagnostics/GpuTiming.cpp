#include "Diagnostics/GpuTiming.h"
#include "Rendering/Core/VulkanContext.h"
#include "Rendering/Core/VulkanCheck.h"
#include <vector>

namespace VulkanRenderer {
GpuTiming::GpuTiming(const VulkanContext &context) : _context(context) {
}

GpuTiming::~GpuTiming() noexcept {
	if (_pool) {
		vkDestroyQueryPool(_context.GetDevice(), _pool, nullptr);
	}
}

void GpuTiming::Initialize(bool enabled) {
	if (!enabled) {
		return;
	}
	if (_pool) {
		throw std::logic_error("GPU timing already initialized");
	}
	VkPhysicalDeviceProperties properties{};
	vkGetPhysicalDeviceProperties(_context.GetPhysicalDevice(), &properties);
	uint32_t count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(_context.GetPhysicalDevice(), &count, nullptr);
	std::vector<VkQueueFamilyProperties> families(count);
	vkGetPhysicalDeviceQueueFamilyProperties(_context.GetPhysicalDevice(), &count, families.data());
	_validBits = families.at(_context.GetGraphicsFamily()).timestampValidBits;
	_period = properties.limits.timestampPeriod;
	if (!_validBits || !properties.limits.timestampComputeAndGraphics) {
		return;
	}
	VkQueryPoolCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
	info.queryType = VK_QUERY_TYPE_TIMESTAMP;
	info.queryCount = 2;
	CheckVulkan(vkCreateQueryPool(_context.GetDevice(), &info, nullptr, &_pool), "failed to create timestamp pool");
}

void GpuTiming::Begin(VkCommandBuffer commandBuffer) {
	if (!_pool) {
		return;
	}
	vkCmdResetQueryPool(commandBuffer, _pool, 0, 2);
	vkCmdWriteTimestamp2(commandBuffer, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, _pool, 0);
}

void GpuTiming::End(VkCommandBuffer commandBuffer) {
	if (!_pool) {
		return;
	}
	vkCmdWriteTimestamp2(commandBuffer, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, _pool, 1);
	_recorded = true;
}

std::optional<double> GpuTiming::ReadMilliseconds() {
	if (!_recorded) {
		return std::nullopt;
	}
	_recorded = false;
	uint64_t values[2]{};
	const auto RESULT = vkGetQueryPoolResults(_context.GetDevice(), _pool, 0, 2, sizeof(values), values,
											  sizeof(uint64_t), VK_QUERY_RESULT_64_BIT);
	if (RESULT == VK_NOT_READY) {
		return std::nullopt;
	}
	CheckVulkan(RESULT, "failed to read GPU timestamps");
	const uint64_t MASK = _validBits == 64 ? UINT64_MAX : (uint64_t{1} << _validBits) - 1;
	return static_cast<double>((values[1] - values[0]) & MASK) * _period / 1000000.0;
}
} // namespace VulkanRenderer
