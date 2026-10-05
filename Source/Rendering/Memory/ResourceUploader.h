#pragma once
#include <vulkan/vulkan.h>
#include <memory>
#include <span>
#include <cstdint>

namespace VulkanRenderer {
class VulkanContext;
class MemoryAllocator;
class AllocatedBuffer;
class AllocatedImage;

/** Uploads VMA staging data into device-local buffers and images; waits only during resource uploads. */
class ResourceUploader {
public:
	ResourceUploader(const VulkanContext &context, const MemoryAllocator &allocator);
	virtual ~ResourceUploader() noexcept;
	void Initialize();
	void Upload(const AllocatedBuffer &destination, const void *data, VkDeviceSize size,
				VkPipelineStageFlags2 destinationStage, VkAccessFlags2 destinationAccess);

	/** Uploads a complete RGBA8 mip chain to a fresh image. */
	void UploadImage(const AllocatedImage &destination, std::span<const uint8_t> pixels,
					 std::span<const VkBufferImageCopy> regions);
	ResourceUploader(const ResourceUploader &) = delete;
	ResourceUploader &operator=(const ResourceUploader &) = delete;

private:
	void BeginUpload(const void *data, VkDeviceSize size);
	void FinishUpload();
	const VulkanContext &_context;
	const MemoryAllocator &_allocator;
	VkCommandPool _commandPool = VK_NULL_HANDLE;
	VkCommandBuffer _commandBuffer = VK_NULL_HANDLE;
	VkFence _fence = VK_NULL_HANDLE;
	bool _pending = false;
	std::unique_ptr<AllocatedBuffer> _staging;
};
} // namespace VulkanRenderer
