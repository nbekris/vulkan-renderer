#pragma once
#include <array>
#include <cstdint>
#include "Rendering/Frames/FrameResources.h"
#include "Rendering/IDrawCommands.h"
#include "Rendering/Memory/MemoryAllocator.h"
#include "Rendering/Presentation/SwapChain.h"
#include "Rendering/Core/VulkanCheck.h"
#include "Rendering/Core/VulkanContext.h"

namespace VulkanRenderer {
/** Renders to a VMA image and reads pixels to verify actual shader output. */
class RenderProbe {
public:
	RenderProbe(const VulkanContext &context, const MemoryAllocator &allocator, const SwapChain &swapChain)
		: _context(context), _allocator(allocator), _extent(swapChain.GetExtent()),
		  _format(swapChain.GetImageFormat()) {}

	virtual ~RenderProbe() noexcept {
		vkDeviceWaitIdle(_context.GetDevice());
		if (_view) {
			vkDestroyImageView(_context.GetDevice(), _view, nullptr);
		}
		if (_image) {
			vmaDestroyImage(_allocator.GetHandle(), _image, _imageAllocation);
		}
		if (_readback) {
			vmaDestroyBuffer(_allocator.GetHandle(), _readback, _readbackAllocation);
		}
	}

	RenderProbe(const RenderProbe &) = delete;
	RenderProbe &operator=(const RenderProbe &) = delete;

	void Initialize() {
		if (_format != VK_FORMAT_B8G8R8A8_SRGB && _format != VK_FORMAT_B8G8R8A8_UNORM
			&& _format != VK_FORMAT_R8G8B8A8_SRGB && _format != VK_FORMAT_R8G8B8A8_UNORM) {
			throw std::runtime_error("render probe requires an 8-bit RGBA/BGRA surface");
		}
		VkImageCreateInfo image{};
		image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		image.imageType = VK_IMAGE_TYPE_2D;
		image.format = _format;
		image.extent = {_extent.width, _extent.height, 1};
		image.mipLevels = 1;
		image.arrayLayers = 1;
		image.samples = VK_SAMPLE_COUNT_1_BIT;
		image.tiling = VK_IMAGE_TILING_OPTIMAL;
		image.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		VmaAllocationCreateInfo allocation{};
		allocation.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
		CheckVulkan(vmaCreateImage(_allocator.GetHandle(), &image, &allocation, &_image, &_imageAllocation, nullptr),
					"failed to create probe image");
		VkImageViewCreateInfo view{};
		view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		view.image = _image;
		view.viewType = VK_IMAGE_VIEW_TYPE_2D;
		view.format = _format;
		view.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
		CheckVulkan(vkCreateImageView(_context.GetDevice(), &view, nullptr, &_view), "failed to create probe view");
		VkBufferCreateInfo buffer{};
		buffer.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		buffer.size = static_cast<VkDeviceSize>(_extent.width) * _extent.height * 4;
		buffer.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		allocation.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
		allocation.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
		VmaAllocationInfo mapped{};
		CheckVulkan(
			vmaCreateBuffer(_allocator.GetHandle(), &buffer, &allocation, &_readback, &_readbackAllocation, &mapped),
			"failed to create probe readback");
		_pixels = static_cast<const unsigned char *>(mapped.pMappedData);
	}

	void Render(const IDrawCommands &triangle, FrameResources &frame, uint32_t frameIndex) {
		const VkCommandBuffer COMMAND = frame.GetCommandBuffer();
		VkCommandBufferBeginInfo begin{};
		begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		CheckVulkan(vkBeginCommandBuffer(COMMAND, &begin), "failed to begin probe");
		Transition(COMMAND, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				   VK_PIPELINE_STAGE_2_NONE, 0, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
				   VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);
		const auto &DEPTH = frame.GetDepthTarget();
		const bool HAS_DEPTH = DEPTH.GetFormat() != VK_FORMAT_UNDEFINED;
		if (HAS_DEPTH) {
			DEPTH.Prepare(COMMAND);
		}
		const auto DEPTH_ATTACHMENT = DEPTH.GetAttachment();
		VkRenderingAttachmentInfo attachment{};
		attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		attachment.imageView = _view;
		attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		VkRenderingInfo rendering{};
		rendering.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		rendering.renderArea.extent = _extent;
		rendering.layerCount = 1;
		rendering.colorAttachmentCount = 1;
		rendering.pColorAttachments = &attachment;
		rendering.pDepthAttachment = HAS_DEPTH ? &DEPTH_ATTACHMENT : nullptr;
		vkCmdBeginRendering(COMMAND, &rendering);
		triangle.Record(COMMAND, frameIndex);
		vkCmdEndRendering(COMMAND);
		Transition(COMMAND, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				   VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
				   VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_READ_BIT);
		VkBufferImageCopy copy{};
		copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
		copy.imageExtent = {_extent.width, _extent.height, 1};
		vkCmdCopyImageToBuffer(COMMAND, _image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, _readback, 1, &copy);
		VkMemoryBarrier2 host{};
		host.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
		host.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
		host.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
		host.dstStageMask = VK_PIPELINE_STAGE_2_HOST_BIT;
		host.dstAccessMask = VK_ACCESS_2_HOST_READ_BIT;
		VkDependencyInfo dependency{};
		dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		dependency.memoryBarrierCount = 1;
		dependency.pMemoryBarriers = &host;
		vkCmdPipelineBarrier2(COMMAND, &dependency);
		CheckVulkan(vkEndCommandBuffer(COMMAND), "failed to end probe");
		const VkFence FENCE = frame.GetFence();
		CheckVulkan(vkResetFences(_context.GetDevice(), 1, &FENCE), "failed to reset probe fence");
		VkCommandBufferSubmitInfo command{};
		command.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
		command.commandBuffer = COMMAND;
		VkSubmitInfo2 submit{};
		submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
		submit.commandBufferInfoCount = 1;
		submit.pCommandBufferInfos = &command;
		CheckVulkan(vkQueueSubmit2(_context.GetGraphicsQueue(), 1, &submit, FENCE), "failed to submit probe");
		frame.MarkSubmitted();
		frame.Wait();
		CheckVulkan(vmaInvalidateAllocation(_allocator.GetHandle(), _readbackAllocation, 0, VK_WHOLE_SIZE),
					"failed to invalidate probe readback");
	}

	std::array<uint8_t, 4> GetCenterRgba() const {
		const size_t PIXEL = (static_cast<size_t>(_extent.height / 2) * _extent.width + _extent.width / 2) * 4;
		const bool BGRA = _format == VK_FORMAT_B8G8R8A8_SRGB || _format == VK_FORMAT_B8G8R8A8_UNORM;
		return {_pixels[PIXEL + (BGRA ? 2 : 0)], _pixels[PIXEL + 1], _pixels[PIXEL + (BGRA ? 0 : 2)],
				_pixels[PIXEL + 3]};
	}

	bool HasGreenCenter() const {
		const size_t PIXEL = (static_cast<size_t>(_extent.height / 2) * _extent.width + _extent.width / 2) * 4;
		return _pixels[PIXEL] < 3 && _pixels[PIXEL + 1] > 16 && _pixels[PIXEL + 2] < 3;
	}

	bool HasMulticolorCenter() const {
		const size_t PIXEL = (static_cast<size_t>(_extent.height / 2) * _extent.width + _extent.width / 2) * 4;
		return _pixels[PIXEL] > 16 && _pixels[PIXEL + 1] > 16 && _pixels[PIXEL + 2] > 16;
	}

private:
	void Transition(VkCommandBuffer command, VkImageLayout oldLayout, VkImageLayout newLayout,
					VkPipelineStageFlags2 sourceStage, VkAccessFlags2 sourceAccess,
					VkPipelineStageFlags2 destinationStage, VkAccessFlags2 destinationAccess) const {
		VkImageMemoryBarrier2 barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcStageMask = sourceStage;
		barrier.srcAccessMask = sourceAccess;
		barrier.dstStageMask = destinationStage;
		barrier.dstAccessMask = destinationAccess;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = _image;
		barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
		VkDependencyInfo dependency{};
		dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		dependency.imageMemoryBarrierCount = 1;
		dependency.pImageMemoryBarriers = &barrier;
		vkCmdPipelineBarrier2(command, &dependency);
	}

	const VulkanContext &_context;
	const MemoryAllocator &_allocator;
	VkExtent2D _extent;
	VkFormat _format;
	VkImage _image = VK_NULL_HANDLE;
	VmaAllocation _imageAllocation = nullptr;
	VkImageView _view = VK_NULL_HANDLE;
	VkBuffer _readback = VK_NULL_HANDLE;
	VmaAllocation _readbackAllocation = nullptr;
	const unsigned char *_pixels = nullptr;
};

} // namespace VulkanRenderer
