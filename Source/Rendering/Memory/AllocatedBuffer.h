#pragma once
#include "Rendering/Memory/VmaConfig.h"

namespace VulkanRenderer {
class MemoryAllocator;
enum class BufferMemory { HostWrite, DeviceLocal };

/** Owns a VMA suballocation, supporting mapped host writes and device-local storage. */
class AllocatedBuffer {
public:
	explicit AllocatedBuffer(const MemoryAllocator &allocator);
	virtual ~AllocatedBuffer() noexcept;
	AllocatedBuffer(const AllocatedBuffer &) = delete;
	AllocatedBuffer &operator=(const AllocatedBuffer &) = delete;

	VkBuffer GetHandle() const noexcept { return _buffer; }

	VkDeviceSize GetSize() const noexcept { return _size; }

	VkDeviceAddress GetDeviceAddress() const;
	void Initialize(VkDeviceSize size, VkBufferUsageFlags usage, BufferMemory memory = BufferMemory::HostWrite);
	void Upload(const void *data, VkDeviceSize size);

private:
	const MemoryAllocator &_allocator;
	VkBuffer _buffer = VK_NULL_HANDLE;
	VmaAllocation _allocation = nullptr;
	void *_mappedData = nullptr;
	VkDeviceSize _size = 0;
	VkBufferUsageFlags _usage = 0;
};
} // namespace VulkanRenderer
