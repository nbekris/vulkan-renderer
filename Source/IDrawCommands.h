#pragma once
#include <vulkan/vulkan.h>

namespace VulkanRenderer {
/** Only records scene drawing inside an active render pass; owns no frame scheduling. */
class IDrawCommands {
public:
	IDrawCommands() = default;
	virtual ~IDrawCommands() = default;
	IDrawCommands(const IDrawCommands &) = delete;
	IDrawCommands &operator=(const IDrawCommands &) = delete;
	virtual void Record(VkCommandBuffer commandBuffer) const = 0;
};
} // namespace VulkanRenderer
