#include "FrameRenderer.h"
#include "VulkanContext.h"
#include "SwapChain.h"
#include "RenderTargets.h"
#include "IDrawCommands.h"
#include "VulkanCheck.h"

namespace VulkanRenderer {

FrameRenderer::FrameRenderer(const VulkanContext &context, const SwapChain &swapChain, const RenderTargets &targets,
							 const IDrawCommands &drawCommands)
	: _context(context), _swapChain(swapChain), _targets(targets), _drawCommands(drawCommands) {
}

FrameRenderer::~FrameRenderer() noexcept {
	// Wait before freeing commands or allowing dependencies to be destroyed during unwinding.
	vkDeviceWaitIdle(_context.GetDevice());
	for (auto fence : _inFlightFences) {
		if (fence) {
			vkDestroyFence(_context.GetDevice(), fence, nullptr);
		}
	}
	for (auto semaphore : _renderFinishedSemaphores) {
		if (semaphore) {
			vkDestroySemaphore(_context.GetDevice(), semaphore, nullptr);
		}
	}
	for (auto semaphore : _imageAvailableSemaphores) {
		if (semaphore) {
			vkDestroySemaphore(_context.GetDevice(), semaphore, nullptr);
		}
	}
	if (_commandPool) {
		vkDestroyCommandPool(_context.GetDevice(), _commandPool, nullptr);
	}
}

void FrameRenderer::Initialize() {
	if (_commandPool) {
		throw std::logic_error("frame renderer already initialized");
	}
	CreateCommandPool();
	CreateCommandBuffers();
	CreateSyncObjects();
}

void FrameRenderer::CreateCommandPool() {
	VkCommandPoolCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	info.queueFamilyIndex = _context.GetGraphicsFamily();
	if (vkCreateCommandPool(_context.GetDevice(), &info, nullptr, &_commandPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create command pool");
	}
}

void FrameRenderer::CreateCommandBuffers() {
	_commandBuffers.resize(_MAX_FRAMES_IN_FLIGHT);
	VkCommandBufferAllocateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	info.commandPool = _commandPool;
	info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	info.commandBufferCount = static_cast<uint32_t>(_commandBuffers.size());
	if (vkAllocateCommandBuffers(_context.GetDevice(), &info, _commandBuffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffers");
	}
}

void FrameRenderer::RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) const {
	VkCommandBufferBeginInfo begin{};
	begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	if (vkBeginCommandBuffer(commandBuffer, &begin) != VK_SUCCESS) {
		throw std::runtime_error("failed to begin command buffer");
	}
	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = _targets.GetRenderPass();
	renderPassInfo.framebuffer = _targets.GetFramebuffer(imageIndex);
	renderPassInfo.renderArea.extent = _swapChain.GetExtent();
	const VkClearValue CLEAR = {{{0.02f, 0.02f, 0.04f, 1.0f}}};
	renderPassInfo.clearValueCount = 1;
	renderPassInfo.pClearValues = &CLEAR;
	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	_drawCommands.Record(commandBuffer);
	vkCmdEndRenderPass(commandBuffer);
	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to record command buffer");
	}
}

void FrameRenderer::CreateSyncObjects() {
	_imageAvailableSemaphores.resize(_MAX_FRAMES_IN_FLIGHT);
	_renderFinishedSemaphores.resize(_swapChain.GetImageViews().size());
	_inFlightFences.resize(_MAX_FRAMES_IN_FLIGHT);
	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	for (size_t i = 0; i < _MAX_FRAMES_IN_FLIGHT; ++i) {
		if (vkCreateSemaphore(_context.GetDevice(), &semaphoreInfo, nullptr, &_imageAvailableSemaphores[i])
				!= VK_SUCCESS
			|| vkCreateFence(_context.GetDevice(), &fenceInfo, nullptr, &_inFlightFences[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create synchronization objects");
		}
	}
	// Presentation completion is tied to reacquiring an image, not a frame fence.
	for (auto &semaphore : _renderFinishedSemaphores) {
		if (vkCreateSemaphore(_context.GetDevice(), &semaphoreInfo, nullptr, &semaphore) != VK_SUCCESS) {
			throw std::runtime_error("failed to create presentation semaphore");
		}
	}
}

bool FrameRenderer::DrawFrame() {
	CheckVulkan(vkWaitForFences(_context.GetDevice(), 1, &_inFlightFences[_currentFrame], VK_TRUE, UINT64_MAX),
				"failed to wait for frame fence");
	uint32_t imageIndex;
	const VkResult ACQUIRED
		= vkAcquireNextImageKHR(_context.GetDevice(), _swapChain.GetHandle(), UINT64_MAX,
								_imageAvailableSemaphores[_currentFrame], VK_NULL_HANDLE, &imageIndex);
	// Keep the fence signaled if acquisition cannot submit work for this frame.
	if (ACQUIRED == VK_ERROR_OUT_OF_DATE_KHR) {
		return false;
	}
	if (ACQUIRED != VK_SUCCESS && ACQUIRED != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("failed to acquire swap-chain image");
	}
	CheckVulkan(vkResetCommandBuffer(_commandBuffers[_currentFrame], 0), "failed to reset command buffer");
	RecordCommandBuffer(_commandBuffers[_currentFrame], imageIndex);
	CheckVulkan(vkResetFences(_context.GetDevice(), 1, &_inFlightFences[_currentFrame]), "failed to reset frame fence");
	const VkSemaphore WAIT_SEMAPHORES[] = {_imageAvailableSemaphores[_currentFrame]};
	const VkPipelineStageFlags WAIT_STAGES[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	const VkSemaphore SIGNAL_SEMAPHORES[] = {_renderFinishedSemaphores[imageIndex]};
	VkSubmitInfo submit{};
	submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit.waitSemaphoreCount = 1;
	submit.pWaitSemaphores = WAIT_SEMAPHORES;
	submit.pWaitDstStageMask = WAIT_STAGES;
	submit.commandBufferCount = 1;
	submit.pCommandBuffers = &_commandBuffers[_currentFrame];
	submit.signalSemaphoreCount = 1;
	submit.pSignalSemaphores = SIGNAL_SEMAPHORES;
	if (vkQueueSubmit(_context.GetGraphicsQueue(), 1, &submit, _inFlightFences[_currentFrame]) != VK_SUCCESS) {
		throw std::runtime_error("failed to submit draw commands");
	}
	VkPresentInfoKHR present{};
	present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present.waitSemaphoreCount = 1;
	present.pWaitSemaphores = SIGNAL_SEMAPHORES;
	present.swapchainCount = 1;
	const VkSwapchainKHR SWAP_CHAIN = _swapChain.GetHandle();
	present.pSwapchains = &SWAP_CHAIN;
	present.pImageIndices = &imageIndex;
	const VkResult PRESENTED = vkQueuePresentKHR(_context.GetPresentQueue(), &present);
	if (PRESENTED != VK_SUCCESS && PRESENTED != VK_SUBOPTIMAL_KHR && PRESENTED != VK_ERROR_OUT_OF_DATE_KHR) {
		throw std::runtime_error("failed to present swap-chain image");
	}
	_currentFrame = (_currentFrame + 1) % _MAX_FRAMES_IN_FLIGHT;
	// Suboptimal acquisition still consumes its semaphore through submission/presentation.
	return ACQUIRED == VK_SUCCESS && PRESENTED == VK_SUCCESS;
}
} // namespace VulkanRenderer
