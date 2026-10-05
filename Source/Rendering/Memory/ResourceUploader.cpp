#include "Rendering/Memory/ResourceUploader.h"
#include "Rendering/Memory/AllocatedBuffer.h"
#include "Rendering/Memory/AllocatedImage.h"
#include <algorithm>
#include "Rendering/Core/VulkanContext.h"
#include "Rendering/Core/VulkanCheck.h"

namespace VulkanRenderer {

ResourceUploader::ResourceUploader(const VulkanContext &context, const MemoryAllocator &allocator)
	: _context(context), _allocator(allocator) {
}

ResourceUploader::~ResourceUploader() noexcept {
	if (_pending) {
		vkWaitForFences(_context.GetDevice(), 1, &_fence, VK_TRUE, UINT64_MAX);
	}
	if (_fence) {
		vkDestroyFence(_context.GetDevice(), _fence, nullptr);
	}
	if (_commandPool) {
		vkDestroyCommandPool(_context.GetDevice(), _commandPool, nullptr);
	}
}

void ResourceUploader::Initialize() {
	if (_commandPool) {
		throw std::logic_error("uploader already initialized");
	}
	VkCommandPoolCreateInfo pool{};
	pool.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool.queueFamilyIndex = _context.GetGraphicsFamily();
	CheckVulkan(vkCreateCommandPool(_context.GetDevice(), &pool, nullptr, &_commandPool),
				"failed to create upload pool");
	VkCommandBufferAllocateInfo commands{};
	commands.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commands.commandPool = _commandPool;
	commands.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commands.commandBufferCount = 1;
	CheckVulkan(vkAllocateCommandBuffers(_context.GetDevice(), &commands, &_commandBuffer),
				"failed to allocate upload command");
	VkFenceCreateInfo fence{};
	fence.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	CheckVulkan(vkCreateFence(_context.GetDevice(), &fence, nullptr, &_fence), "failed to create upload fence");
}

void ResourceUploader::Upload(const AllocatedBuffer &destination, const void *data, VkDeviceSize size,
							  VkPipelineStageFlags2 destinationStage, VkAccessFlags2 destinationAccess) {
	if (!_commandPool || size > destination.GetSize()) {
		throw std::invalid_argument("invalid staged upload");
	}
	BeginUpload(data, size);
	const VkBufferCopy COPY{0, 0, size};
	vkCmdCopyBuffer(_commandBuffer, _staging->GetHandle(), destination.GetHandle(), 1, &COPY);
	VkBufferMemoryBarrier2 barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
	barrier.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
	barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
	barrier.dstStageMask = destinationStage;
	barrier.dstAccessMask = destinationAccess;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.buffer = destination.GetHandle();
	barrier.size = size;
	VkDependencyInfo dependency{};
	dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	dependency.bufferMemoryBarrierCount = 1;
	dependency.pBufferMemoryBarriers = &barrier;
	vkCmdPipelineBarrier2(_commandBuffer, &dependency);
	FinishUpload();
}

void ResourceUploader::BeginUpload(const void *data, VkDeviceSize size) {
	if (!_commandPool || _pending || !data || size == 0) {
		throw std::invalid_argument("invalid staged upload data or state");
	}
	_staging = std::make_unique<AllocatedBuffer>(_allocator);
	_staging->Initialize(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
	_staging->Upload(data, size);
	CheckVulkan(vkResetCommandPool(_context.GetDevice(), _commandPool, 0), "failed to reset upload pool");
	CheckVulkan(vkResetFences(_context.GetDevice(), 1, &_fence), "failed to reset upload fence");
	VkCommandBufferBeginInfo begin{};
	begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	CheckVulkan(vkBeginCommandBuffer(_commandBuffer, &begin), "failed to begin upload command");
}

void ResourceUploader::FinishUpload() {
	CheckVulkan(vkEndCommandBuffer(_commandBuffer), "failed to end upload command");
	VkCommandBufferSubmitInfo command{};
	command.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
	command.commandBuffer = _commandBuffer;
	VkSubmitInfo2 submit{};
	submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
	submit.commandBufferInfoCount = 1;
	submit.pCommandBufferInfos = &command;
	CheckVulkan(vkQueueSubmit2(_context.GetGraphicsQueue(), 1, &submit, _fence), "failed to submit upload");
	_pending = true;
	CheckVulkan(vkWaitForFences(_context.GetDevice(), 1, &_fence, VK_TRUE, UINT64_MAX), "failed to wait for upload");
	_pending = false;
	_staging.reset();
}

void ResourceUploader::UploadImage(const AllocatedImage &destination, std::span<const uint8_t> pixels,
								   std::span<const VkBufferImageCopy> regions) {
	if (!destination.GetHandle() || regions.size() != destination.GetMipLevels() || regions.empty()) {
		throw std::invalid_argument("image upload requires all mip levels");
	}
	for (size_t i = 0; i < regions.size(); ++i) {
		const auto &REGION = regions[i];
		const uint32_t WIDTH = std::max(1u, destination.GetExtent().width >> i);
		const uint32_t HEIGHT = std::max(1u, destination.GetExtent().height >> i);
		const VkDeviceSize BYTES = static_cast<VkDeviceSize>(WIDTH) * HEIGHT * 4;
		if (REGION.bufferOffset > pixels.size() || BYTES > pixels.size() - REGION.bufferOffset
			|| REGION.bufferOffset % 4 != 0 || REGION.bufferRowLength != 0 || REGION.bufferImageHeight != 0
			|| REGION.imageSubresource.mipLevel != i || REGION.imageSubresource.aspectMask != VK_IMAGE_ASPECT_COLOR_BIT
			|| REGION.imageSubresource.baseArrayLayer != 0 || REGION.imageSubresource.layerCount != 1
			|| REGION.imageOffset.x != 0 || REGION.imageOffset.y != 0 || REGION.imageOffset.z != 0
			|| REGION.imageExtent.width != WIDTH || REGION.imageExtent.height != HEIGHT
			|| REGION.imageExtent.depth != 1) {
			throw std::invalid_argument("invalid RGBA8 mip upload region");
		}
	}
	BeginUpload(pixels.data(), pixels.size());
	VkImageMemoryBarrier2 barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
	barrier.dstStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
	barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
	barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = destination.GetHandle();
	barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, destination.GetMipLevels(), 0, 1};
	VkDependencyInfo dependency{};
	dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	dependency.imageMemoryBarrierCount = 1;
	dependency.pImageMemoryBarriers = &barrier;
	vkCmdPipelineBarrier2(_commandBuffer, &dependency);
	vkCmdCopyBufferToImage(_commandBuffer, _staging->GetHandle(), destination.GetHandle(),
						   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<uint32_t>(regions.size()), regions.data());
	barrier.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
	barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
	barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
	barrier.dstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	vkCmdPipelineBarrier2(_commandBuffer, &dependency);
	FinishUpload();
}

} // namespace VulkanRenderer
