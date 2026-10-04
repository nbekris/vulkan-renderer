#pragma once
#include "VmaConfig.h"

namespace VulkanRenderer {
class MemoryAllocator;

/** Owns a host-writable buffer and allocation as one resource. */
class AllocatedBuffer {
public:
	explicit AllocatedBuffer(const MemoryAllocator &allocator);
	virtual ~AllocatedBuffer() noexcept;
	AllocatedBuffer(const AllocatedBuffer &) = delete;
	AllocatedBuffer &operator=(const AllocatedBuffer &) = delete;

	VkBuffer GetHandle() const noexcept { return _buffer; }

	void Initialize(VkDeviceSize size, VkBufferUsageFlags usage);
	void Upload(const void *data, VkDeviceSize size);

private:
	const MemoryAllocator &_allocator;
	VkBuffer _buffer = VK_NULL_HANDLE;
	VmaAllocation _allocation = nullptr;
	void *_mappedData = nullptr;
	VkDeviceSize _size = 0;
};
} // namespace VulkanRenderer
