#pragma once
#include <vulkan/vulkan.h>
#include <optional>

namespace VulkanRenderer {
class VulkanContext;

/** Optional per-frame timestamps; results are read only after that frame's fence. */
class GpuTiming {
public:
	explicit GpuTiming(const VulkanContext &context);
	virtual ~GpuTiming() noexcept;
	GpuTiming(const GpuTiming &) = delete;
	GpuTiming &operator=(const GpuTiming &) = delete;
	void Initialize(bool enabled);
	void Begin(VkCommandBuffer commandBuffer);
	void End(VkCommandBuffer commandBuffer);
	std::optional<double> ReadMilliseconds();

private:
	const VulkanContext &_context;
	VkQueryPool _pool = VK_NULL_HANDLE;
	uint32_t _validBits = 0;
	float _period = 0;
	bool _recorded = false;
};
} // namespace VulkanRenderer
