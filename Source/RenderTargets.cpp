#include "RenderTargets.h"
#include "VulkanContext.h"
#include "SwapChain.h"
#include <stdexcept>

namespace VulkanRenderer {

RenderTargets::RenderTargets(const VulkanContext &context, const SwapChain &swapChain)
	: _context(context), _swapChain(swapChain) {
}

RenderTargets::~RenderTargets() noexcept {
	for (auto framebuffer : _framebuffers) {
		if (framebuffer) {
			vkDestroyFramebuffer(_context.GetDevice(), framebuffer, nullptr);
		}
	}
	if (_renderPass) {
		vkDestroyRenderPass(_context.GetDevice(), _renderPass, nullptr);
	}
}

void RenderTargets::Initialize() {
	if (_renderPass) {
		throw std::logic_error("render targets already initialized");
	}
	CreateRenderPass();
	CreateFramebuffers();
}

void RenderTargets::CreateRenderPass() {
	VkAttachmentDescription color{};
	color.format = _swapChain.GetImageFormat();
	color.samples = VK_SAMPLE_COUNT_1_BIT;
	color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	color.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	color.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	color.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	VkAttachmentReference colorRef{};
	colorRef.attachment = 0;
	colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorRef;
	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	VkRenderPassCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	info.attachmentCount = 1;
	info.pAttachments = &color;
	info.subpassCount = 1;
	info.pSubpasses = &subpass;
	info.dependencyCount = 1;
	info.pDependencies = &dependency;
	if (vkCreateRenderPass(_context.GetDevice(), &info, nullptr, &_renderPass) != VK_SUCCESS) {
		throw std::runtime_error("failed to create render pass");
	}
}

void RenderTargets::CreateFramebuffers() {
	_framebuffers.resize(_swapChain.GetImageViews().size());
	for (size_t i = 0; i < _swapChain.GetImageViews().size(); ++i) {
		const VkImageView ATTACHMENTS[] = {_swapChain.GetImageViews()[i]};
		VkFramebufferCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		info.renderPass = _renderPass;
		info.attachmentCount = 1;
		info.pAttachments = ATTACHMENTS;
		info.width = _swapChain.GetExtent().width;
		info.height = _swapChain.GetExtent().height;
		info.layers = 1;
		if (vkCreateFramebuffer(_context.GetDevice(), &info, nullptr, &_framebuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create framebuffer");
		}
	}
}
} // namespace VulkanRenderer
