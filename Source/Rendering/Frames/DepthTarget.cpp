#include "Rendering/Frames/DepthTarget.h"
#include "Rendering/Core/VulkanContext.h"
#include <stdexcept>

namespace VulkanRenderer {
VkFormat DepthTarget::SelectFormat(const VulkanContext &context) {
	constexpr VkFormat CANDIDATES[]
		= {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D16_UNORM, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};
	for (auto format : CANDIDATES) {
		VkFormatProperties properties{};
		vkGetPhysicalDeviceFormatProperties(context.GetPhysicalDevice(), format, &properties);
		if (properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
			return format;
		}
	}
	throw std::runtime_error("device has no supported depth attachment format");
}

DepthTarget::DepthTarget(const MemoryAllocator &allocator) : _image(allocator) {
}

void DepthTarget::Initialize(VkExtent2D extent, VkFormat format) {
	if (format != VK_FORMAT_D32_SFLOAT && format != VK_FORMAT_D16_UNORM && format != VK_FORMAT_D32_SFLOAT_S8_UINT
		&& format != VK_FORMAT_D24_UNORM_S8_UINT) {
		throw std::invalid_argument("unsupported depth target format");
	}
	_image.Initialize(extent, format, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);
	_format = format;
}

VkRenderingAttachmentInfo DepthTarget::GetAttachment() const {
	VkRenderingAttachmentInfo attachment{};
	attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	attachment.imageView = _image.GetView();
	attachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachment.clearValue.depthStencil = {1.0f, 0};
	return attachment;
}

void DepthTarget::Prepare(VkCommandBuffer commandBuffer) const {
	if (!_image.GetView()) {
		throw std::logic_error("depth target is not initialized");
	}
	VkImageMemoryBarrier2 barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
	barrier.srcStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
	barrier.srcAccessMask
		= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	barrier.dstStageMask = barrier.srcStageMask;
	barrier.dstAccessMask = barrier.srcAccessMask;
	barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = _image.GetHandle();
	barrier.subresourceRange = {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1};
	// Transition both aspects for combined formats without requiring separateDepthStencilLayouts.
	if (_format == VK_FORMAT_D32_SFLOAT_S8_UINT || _format == VK_FORMAT_D24_UNORM_S8_UINT) {
		barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
	}
	VkDependencyInfo dependency{};
	dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	dependency.imageMemoryBarrierCount = 1;
	dependency.pImageMemoryBarriers = &barrier;
	vkCmdPipelineBarrier2(commandBuffer, &dependency);
}
} // namespace VulkanRenderer
