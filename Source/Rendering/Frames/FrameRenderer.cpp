#include "Rendering/Frames/FrameRenderer.h"
#include "Rendering/Core/VulkanContext.h"
#include "Rendering/Presentation/SwapChain.h"
#include "Rendering/Frames/FrameRing.h"
#include "Rendering/Frames/FrameResources.h"
#include "Rendering/IDrawCommands.h"
#include "Rendering/Core/VulkanCheck.h"

namespace VulkanRenderer {

FrameRenderer::FrameRenderer(const VulkanContext &context, const SwapChain &swapChain, const FrameRing &frames,
							 const IDrawCommands &drawCommands)
	: _context(context), _swapChain(swapChain), _frames(frames), _drawCommands(drawCommands) {
}

FrameRenderer::~FrameRenderer() noexcept {
	vkDeviceWaitIdle(_context.GetDevice());
	for (auto semaphore : _renderFinishedSemaphores) {
		if (semaphore) {
			vkDestroySemaphore(_context.GetDevice(), semaphore, nullptr);
		}
	}
}

void FrameRenderer::Initialize() {
	if (!_renderFinishedSemaphores.empty() || _frames.GetCount() == 0) {
		throw std::logic_error("frame renderer requires a fresh instance and initialized frame ring");
	}
	// A frame fence does not prove presentation completion: these belong to swapchain images.
	_renderFinishedSemaphores.resize(_swapChain.GetImageViews().size());
	VkSemaphoreCreateInfo semaphore{};
	semaphore.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	for (auto &handle : _renderFinishedSemaphores) {
		CheckVulkan(vkCreateSemaphore(_context.GetDevice(), &semaphore, nullptr, &handle),
					"failed to create present semaphore");
	}
}

void FrameRenderer::TransitionImage(VkCommandBuffer commandBuffer, uint32_t imageIndex, bool beforeRendering) const {
	VkImageMemoryBarrier2 barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
	// Every frame clears the attachment, so its previous contents may be discarded.
	barrier.oldLayout = beforeRendering ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	barrier.newLayout = beforeRendering ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	// Chain the acquired-image layout transition to the submit's color-output semaphore wait.
	barrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
	barrier.srcAccessMask = beforeRendering ? VK_ACCESS_2_NONE : VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
	barrier.dstStageMask = beforeRendering ? VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT : VK_PIPELINE_STAGE_2_NONE;
	barrier.dstAccessMask = beforeRendering ? VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT : VK_ACCESS_2_NONE;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = _swapChain.GetImage(imageIndex);
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.layerCount = 1;
	VkDependencyInfo dependency{};
	dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	dependency.imageMemoryBarrierCount = 1;
	dependency.pImageMemoryBarriers = &barrier;
	vkCmdPipelineBarrier2(commandBuffer, &dependency);
}

void FrameRenderer::RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) const {
	VkCommandBufferBeginInfo begin{};
	begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	CheckVulkan(vkBeginCommandBuffer(commandBuffer, &begin), "failed to begin frame command");
	_frames.GetFrame(_currentFrame).GetTiming().Begin(commandBuffer);
	_drawCommands.RecordBeforeRendering(commandBuffer, _currentFrame);
	TransitionImage(commandBuffer, imageIndex, true);
	const auto &DEPTH = _frames.GetFrame(_currentFrame).GetDepthTarget();
	const bool HAS_DEPTH = DEPTH.GetFormat() != VK_FORMAT_UNDEFINED;
	if (HAS_DEPTH) {
		DEPTH.Prepare(commandBuffer);
	}
	const auto DEPTH_ATTACHMENT = DEPTH.GetAttachment();
	VkRenderingAttachmentInfo attachment{};
	attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	attachment.imageView = _swapChain.GetImageViews().at(imageIndex);
	attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachment.clearValue = {{{0.02f, 0.02f, 0.04f, 1.0f}}};
	VkRenderingInfo rendering{};
	rendering.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	rendering.renderArea.extent = _swapChain.GetExtent();
	rendering.layerCount = 1;
	rendering.colorAttachmentCount = 1;
	rendering.pColorAttachments = &attachment;
	rendering.pDepthAttachment = HAS_DEPTH ? &DEPTH_ATTACHMENT : nullptr;
	vkCmdBeginRendering(commandBuffer, &rendering);
	_drawCommands.Record(commandBuffer, _currentFrame);
	vkCmdEndRendering(commandBuffer);
	_frames.GetFrame(_currentFrame).GetTiming().End(commandBuffer);
	TransitionImage(commandBuffer, imageIndex, false);
	CheckVulkan(vkEndCommandBuffer(commandBuffer), "failed to end frame command");
}

void FrameRenderer::SubmitFrame(FrameResources &frame, uint32_t imageIndex) {
	const VkFence FENCE = frame.GetFence();
	CheckVulkan(vkResetFences(_context.GetDevice(), 1, &FENCE), "failed to reset frame fence");
	VkSemaphoreSubmitInfo wait{};
	wait.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	wait.semaphore = frame.GetImageAvailable();
	wait.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSemaphoreSubmitInfo signal{};
	signal.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	signal.semaphore = _renderFinishedSemaphores.at(imageIndex);
	signal.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
	VkCommandBufferSubmitInfo command{};
	command.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
	command.commandBuffer = frame.GetCommandBuffer();
	VkSubmitInfo2 submit{};
	submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
	submit.waitSemaphoreInfoCount = 1;
	submit.pWaitSemaphoreInfos = &wait;
	submit.commandBufferInfoCount = 1;
	submit.pCommandBufferInfos = &command;
	submit.signalSemaphoreInfoCount = 1;
	submit.pSignalSemaphoreInfos = &signal;
	CheckVulkan(vkQueueSubmit2(_context.GetGraphicsQueue(), 1, &submit, FENCE), "failed to submit frame");
	frame.MarkSubmitted();
}

bool FrameRenderer::DrawFrame(const FrameUniform &uniform) {
	auto &frame = _frames.GetFrame(_currentFrame);
	frame.Wait();
	_lastGpuMilliseconds = frame.GetTiming().ReadMilliseconds();
	uint32_t imageIndex = 0;
	const VkResult ACQUIRED = vkAcquireNextImageKHR(_context.GetDevice(), _swapChain.GetHandle(), UINT64_MAX,
													frame.GetImageAvailable(), VK_NULL_HANDLE, &imageIndex);
	if (ACQUIRED == VK_ERROR_OUT_OF_DATE_KHR) {
		return false;
	}
	if (ACQUIRED != VK_SUCCESS && ACQUIRED != VK_SUBOPTIMAL_KHR) {
		CheckVulkan(ACQUIRED, "failed to acquire swapchain image");
	}
	frame.Prepare(uniform);
	RecordCommandBuffer(frame.GetCommandBuffer(), imageIndex);
	SubmitFrame(frame, imageIndex);
	const VkSwapchainKHR SWAP_CHAIN = _swapChain.GetHandle();
	const VkSemaphore SIGNAL = _renderFinishedSemaphores.at(imageIndex);
	VkPresentInfoKHR present{};
	present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present.waitSemaphoreCount = 1;
	present.pWaitSemaphores = &SIGNAL;
	present.swapchainCount = 1;
	present.pSwapchains = &SWAP_CHAIN;
	present.pImageIndices = &imageIndex;
	const VkResult PRESENTED = vkQueuePresentKHR(_context.GetPresentQueue(), &present);
	if (PRESENTED != VK_SUCCESS && PRESENTED != VK_SUBOPTIMAL_KHR && PRESENTED != VK_ERROR_OUT_OF_DATE_KHR) {
		CheckVulkan(PRESENTED, "failed to present swapchain image");
	}
	_currentFrame = (_currentFrame + 1) % _frames.GetCount();
	return ACQUIRED == VK_SUCCESS && PRESENTED == VK_SUCCESS;
}

} // namespace VulkanRenderer
