#include "Rendering/Frames/FrameResources.h"
#include "Rendering/Core/VulkanContext.h"
#include "Rendering/Core/VulkanCheck.h"

namespace VulkanRenderer {

FrameResources::FrameResources(const VulkanContext &context, const MemoryAllocator &allocator)
	: _context(context), _uniformBuffer(allocator), _depthTarget(allocator), _timing(context) {
}

FrameResources::~FrameResources() noexcept {
	if (_pending) {
		vkWaitForFences(_context.GetDevice(), 1, &_fence, VK_TRUE, UINT64_MAX);
	}
	if (_fence) {
		vkDestroyFence(_context.GetDevice(), _fence, nullptr);
	}
	if (_imageAvailable) {
		vkDestroySemaphore(_context.GetDevice(), _imageAvailable, nullptr);
	}
	if (_commandPool) {
		vkDestroyCommandPool(_context.GetDevice(), _commandPool, nullptr);
	}
}

void FrameResources::Initialize(VkExtent2D depthExtent, VkFormat depthFormat, bool profile) {
	if (_commandPool) {
		throw std::logic_error("frame resources already initialized");
	}
	if (depthFormat != VK_FORMAT_UNDEFINED) {
		_depthTarget.Initialize(depthExtent, depthFormat);
	}
	_timing.Initialize(profile);
	_uniformBuffer.Initialize(sizeof(FrameUniform), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
	VkCommandPoolCreateInfo pool{};
	pool.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool.queueFamilyIndex = _context.GetGraphicsFamily();
	CheckVulkan(vkCreateCommandPool(_context.GetDevice(), &pool, nullptr, &_commandPool),
				"failed to create frame pool");
	VkCommandBufferAllocateInfo commands{};
	commands.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commands.commandPool = _commandPool;
	commands.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commands.commandBufferCount = 1;
	CheckVulkan(vkAllocateCommandBuffers(_context.GetDevice(), &commands, &_commandBuffer),
				"failed to allocate frame command");
	VkSemaphoreCreateInfo semaphore{};
	semaphore.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	CheckVulkan(vkCreateSemaphore(_context.GetDevice(), &semaphore, nullptr, &_imageAvailable),
				"failed to create acquire semaphore");
	VkFenceCreateInfo fence{};
	fence.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	CheckVulkan(vkCreateFence(_context.GetDevice(), &fence, nullptr, &_fence), "failed to create frame fence");
}

void FrameResources::Wait() {
	CheckVulkan(vkWaitForFences(_context.GetDevice(), 1, &_fence, VK_TRUE, UINT64_MAX), "failed to wait for frame");
	_pending = false;
}

void FrameResources::Prepare(const FrameUniform &uniform) {
	if (_pending) {
		throw std::logic_error("cannot overwrite a frame still in flight");
	}
	CheckVulkan(vkResetCommandPool(_context.GetDevice(), _commandPool, 0), "failed to reset frame pool");
	_uniformBuffer.Upload(&uniform, sizeof(uniform));
}

} // namespace VulkanRenderer
