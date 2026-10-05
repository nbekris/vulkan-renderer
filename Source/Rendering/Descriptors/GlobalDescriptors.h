#pragma once
#include <vulkan/vulkan.h>

namespace VulkanRenderer {
class VulkanContext;
class SceneResources;
class FrameRing;

/** One immutable descriptor table per session, indexed by per-draw push constants. */
class GlobalDescriptors {
public:
	GlobalDescriptors(const VulkanContext &context, const SceneResources &resources, const FrameRing &frames);
	virtual ~GlobalDescriptors() noexcept;

	VkDescriptorSetLayout GetLayout() const noexcept { return _layout; }

	bool IsMaterialBound(uint32_t index) const noexcept;

	void Bind(VkCommandBuffer commandBuffer, VkPipelineLayout layout) const;
	void Initialize();

	GlobalDescriptors(const GlobalDescriptors &) = delete;
	GlobalDescriptors &operator=(const GlobalDescriptors &) = delete;

private:
	void CreateLayout();
	void CreatePool();
	void WriteDescriptors();
	const VulkanContext &_context;
	const SceneResources &_resources;
	const FrameRing &_frames;
	VkDescriptorSetLayout _layout = VK_NULL_HANDLE;
	VkDescriptorPool _pool = VK_NULL_HANDLE;
	VkDescriptorSet _set = VK_NULL_HANDLE;
};
} // namespace VulkanRenderer
