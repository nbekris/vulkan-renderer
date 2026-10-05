#pragma once
#include <vulkan/vulkan.h>

namespace VulkanRenderer {
/** Only records scene drawing inside active dynamic rendering; owns no frame scheduling. */
class IDrawCommands {
public:
	IDrawCommands() = default;
	virtual ~IDrawCommands() = default;
	IDrawCommands(const IDrawCommands &) = delete;
	IDrawCommands &operator=(const IDrawCommands &) = delete;
	virtual void Record(VkCommandBuffer commandBuffer, uint32_t frameIndex) const = 0;
};
} // namespace VulkanRenderer
