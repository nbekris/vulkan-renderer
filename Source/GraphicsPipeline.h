#pragma once
#include <vulkan/vulkan.h>
#include <span>
#include <string>

namespace VulkanRenderer {
class VulkanContext;

/** Owns a graphics pipeline; shader paths and vertex layout are supplied by the caller. */
class GraphicsPipeline {
public:
	explicit GraphicsPipeline(const VulkanContext &context);
	virtual ~GraphicsPipeline() noexcept;
	GraphicsPipeline(const GraphicsPipeline &) = delete;
	GraphicsPipeline &operator=(const GraphicsPipeline &) = delete;
	void Bind(VkCommandBuffer commandBuffer) const;
	void Initialize(VkRenderPass renderPass, VkExtent2D extent, const VkVertexInputBindingDescription &binding,
					std::span<const VkVertexInputAttributeDescription> attributes, const std::string &vertexShader,
					const std::string &fragmentShader);

private:
	const VulkanContext &_context;
	VkPipelineLayout _layout = VK_NULL_HANDLE;
	VkPipeline _pipeline = VK_NULL_HANDLE;
};
} // namespace VulkanRenderer
