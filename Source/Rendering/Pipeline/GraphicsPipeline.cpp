#include "Rendering/Pipeline/GraphicsPipeline.h"
#include "Rendering/Core/VulkanContext.h"
#include "Rendering/Pipeline/ShaderModule.h"
#include <stdexcept>

namespace VulkanRenderer {

GraphicsPipeline::GraphicsPipeline(const VulkanContext &context) : _context(context) {
}

GraphicsPipeline::~GraphicsPipeline() noexcept {
	if (_pipeline) {
		vkDestroyPipeline(_context.GetDevice(), _pipeline, nullptr);
	}
	if (_layout) {
		vkDestroyPipelineLayout(_context.GetDevice(), _layout, nullptr);
	}
}

void GraphicsPipeline::Bind(VkCommandBuffer commandBuffer) const {
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);
}

void GraphicsPipeline::PushDrawData(VkCommandBuffer commandBuffer, const DrawData &data) const {
	vkCmdPushConstants(commandBuffer, _layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
					   sizeof(data), &data);
}

void GraphicsPipeline::Initialize(VkFormat colorFormat, VkExtent2D extent, const std::string &vertexShader,
								  const std::string &fragmentShader, VkDescriptorSetLayout descriptorLayout,
								  VkFormat depthFormat) {

	if (_pipeline || _layout) {
		throw std::logic_error("pipeline already initialized");
	}
	ShaderModule vertModule(_context.GetDevice());
	ShaderModule fragModule(_context.GetDevice());
	vertModule.Initialize(vertexShader);
	fragModule.Initialize(fragmentShader);
	VkPipelineShaderStageCreateInfo vertStage{};
	vertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertStage.module = vertModule.GetHandle();
	vertStage.pName = "main";
	VkPipelineShaderStageCreateInfo fragStage{};
	fragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragStage.module = fragModule.GetHandle();
	fragStage.pName = "main";
	const VkPipelineShaderStageCreateInfo STAGES[] = {vertStage, fragStage};
	VkPipelineVertexInputStateCreateInfo vertexInput{};
	vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	VkPipelineInputAssemblyStateCreateInfo assembly{};
	assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	VkViewport viewport{};
	viewport.width = static_cast<float>(extent.width);
	viewport.height = static_cast<float>(extent.height);
	viewport.maxDepth = 1.0f;
	VkRect2D scissor{};
	scissor.extent = extent;
	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;
	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	// Meshes are double-sided until material-specific rasterization is introduced.
	rasterizer.cullMode = VK_CULL_MODE_NONE;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	VkPipelineDepthStencilStateCreateInfo depth{};
	depth.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depth.depthTestEnable = depthFormat != VK_FORMAT_UNDEFINED;
	depth.depthWriteEnable = depth.depthTestEnable;
	depth.depthCompareOp = VK_COMPARE_OP_LESS;
	VkPipelineColorBlendAttachmentState blendAttachment{};
	blendAttachment.colorWriteMask
		= VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	VkPipelineColorBlendStateCreateInfo blending{};
	blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	blending.attachmentCount = colorFormat == VK_FORMAT_UNDEFINED ? 0 : 1;
	blending.pAttachments = &blendAttachment;
	VkPipelineLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	const VkPushConstantRange DRAW_RANGE{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
										 sizeof(DrawData)};
	layoutInfo.pushConstantRangeCount = 1;
	layoutInfo.pPushConstantRanges = &DRAW_RANGE;
	if (descriptorLayout) {
		layoutInfo.setLayoutCount = 1;
		layoutInfo.pSetLayouts = &descriptorLayout;
	}
	if (vkCreatePipelineLayout(_context.GetDevice(), &layoutInfo, nullptr, &_layout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout");
	}
	VkGraphicsPipelineCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	VkPipelineRenderingCreateInfo rendering{};
	rendering.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
	rendering.colorAttachmentCount = blending.attachmentCount;
	rendering.pColorAttachmentFormats = &colorFormat;
	rendering.depthAttachmentFormat = depthFormat;
	info.pNext = &rendering;
	info.stageCount = 2;
	info.pStages = STAGES;
	info.pVertexInputState = &vertexInput;
	info.pInputAssemblyState = &assembly;
	info.pViewportState = &viewportState;
	info.pRasterizationState = &rasterizer;
	info.pMultisampleState = &multisampling;
	info.pColorBlendState = &blending;
	info.pDepthStencilState = &depth;
	info.layout = _layout;
	info.renderPass = VK_NULL_HANDLE;
	const VkResult RESULT
		= vkCreateGraphicsPipelines(_context.GetDevice(), VK_NULL_HANDLE, 1, &info, nullptr, &_pipeline);
	if (RESULT != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline");
	}
}
} // namespace VulkanRenderer
