#include "Rendering/Frames/ShadowTarget.h"
#include "Rendering/Core/VulkanContext.h"
#include "Rendering/Core/VulkanCheck.h"
#include <stdexcept>

namespace VulkanRenderer {
ShadowTarget::ShadowTarget(const VulkanContext &context, const MemoryAllocator &allocator)
	: _context(context), _image(allocator) {
}

ShadowTarget::~ShadowTarget() noexcept {
	if (_sampler) {
		vkDestroySampler(_context.GetDevice(), _sampler, nullptr);
	}
}

void ShadowTarget::Initialize() {
	if (_sampler) {
		throw std::logic_error("shadow target already initialized");
	}
	const VkFormatFeatureFlags REQUIRED
		= VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
	for (const auto FORMAT : {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D16_UNORM}) {
		VkFormatProperties properties{};
		vkGetPhysicalDeviceFormatProperties(_context.GetPhysicalDevice(), FORMAT, &properties);
		if ((properties.optimalTilingFeatures & REQUIRED) == REQUIRED) {
			_format = FORMAT;
			break;
		}
	}
	if (_format == VK_FORMAT_UNDEFINED) {
		throw std::runtime_error("no sampled shadow depth format");
	}
	_image.Initialize({RESOLUTION, RESOLUTION}, _format,
					  VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
					  VK_IMAGE_ASPECT_DEPTH_BIT);
	VkSamplerCreateInfo sampler{};
	sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler.magFilter = sampler.minFilter = VK_FILTER_NEAREST;
	sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	sampler.addressModeU = sampler.addressModeV = sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	// Manual comparisons implement an explicit 3x3 PCF kernel without requiring depth linear filtering.
	CheckVulkan(vkCreateSampler(_context.GetDevice(), &sampler, nullptr, &_sampler), "failed to create shadow sampler");
}

VkDescriptorImageInfo ShadowTarget::GetDescriptor() const {
	return {_sampler, _image.GetView(), VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL};
}

VkRenderingAttachmentInfo ShadowTarget::GetAttachment() const {
	VkRenderingAttachmentInfo attachment{};
	attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	attachment.imageView = _image.GetView();
	attachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachment.clearValue.depthStencil = {1, 0};
	return attachment;
}

void ShadowTarget::Transition(VkCommandBuffer command, bool forRendering) const {
	VkImageMemoryBarrier2 barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
	const auto TESTS = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
	barrier.srcStageMask = forRendering ? VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT : TESTS;
	barrier.srcAccessMask
		= forRendering ? VK_ACCESS_2_SHADER_SAMPLED_READ_BIT : VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	barrier.dstStageMask = forRendering ? TESTS : VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
	barrier.dstAccessMask
		= forRendering ? VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
					   : VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
	barrier.oldLayout = forRendering ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	barrier.newLayout = forRendering ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
									 : VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
	barrier.srcQueueFamilyIndex = barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = _image.GetHandle();
	barrier.subresourceRange = {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1};
	VkDependencyInfo dependency{};
	dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	dependency.imageMemoryBarrierCount = 1;
	dependency.pImageMemoryBarriers = &barrier;
	vkCmdPipelineBarrier2(command, &dependency);
}
} // namespace VulkanRenderer
