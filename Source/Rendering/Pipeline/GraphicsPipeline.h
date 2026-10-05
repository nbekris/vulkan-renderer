#pragma once
#include <vulkan/vulkan.h>
#include "Rendering/RenderData.h"
#include <string>

namespace VulkanRenderer {
class VulkanContext;

/** Owns a dynamic-rendering pipeline with caller-supplied shaders, format and descriptor layout. */
class GraphicsPipeline {
public:
	explicit GraphicsPipeline(const VulkanContext &context);
	virtual ~GraphicsPipeline() noexcept;
	GraphicsPipeline(const GraphicsPipeline &) = delete;
	GraphicsPipeline &operator=(const GraphicsPipeline &) = delete;
	void Bind(VkCommandBuffer commandBuffer) const;

	VkPipelineLayout GetLayout() const noexcept { return _layout; }

	void PushDrawData(VkCommandBuffer commandBuffer, const DrawData &data) const;
	void Initialize(VkFormat colorFormat, VkExtent2D extent, const std::string &vertexShader,
					const std::string &fragmentShader, VkDescriptorSetLayout descriptorLayout = VK_NULL_HANDLE,
					VkFormat depthFormat = VK_FORMAT_UNDEFINED);

private:
	const VulkanContext &_context;
	VkPipelineLayout _layout = VK_NULL_HANDLE;
	VkPipeline _pipeline = VK_NULL_HANDLE;
};
} // namespace VulkanRenderer
