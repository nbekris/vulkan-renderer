#include "AllocatedBuffer.h"
#include "MemoryAllocator.h"
#include "VulkanCheck.h"
#include <cstring>

namespace VulkanRenderer {

AllocatedBuffer::AllocatedBuffer(const MemoryAllocator &allocator) : _allocator(allocator) {
}

AllocatedBuffer::~AllocatedBuffer() noexcept {
	if (_buffer) {
		vmaDestroyBuffer(_allocator.GetHandle(), _buffer, _allocation);
	}
}

void AllocatedBuffer::Initialize(VkDeviceSize size, VkBufferUsageFlags usage) {
	if (_buffer) {
		throw std::logic_error("buffer already initialized");
	}
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	VmaAllocationCreateInfo allocationInfo{};
	allocationInfo.usage = VMA_MEMORY_USAGE_AUTO;
	allocationInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
	VmaAllocationInfo mapped{};
	CheckVulkan(vmaCreateBuffer(_allocator.GetHandle(), &bufferInfo, &allocationInfo, &_buffer, &_allocation, &mapped),
				"failed to allocate buffer");
	_mappedData = mapped.pMappedData;
	_size = size;
}

void AllocatedBuffer::Upload(const void *data, VkDeviceSize size) {
	if (!_mappedData || !data || size > _size) {
		throw std::invalid_argument("invalid buffer upload");
	}
	std::memcpy(_mappedData, data, static_cast<size_t>(size));
	CheckVulkan(vmaFlushAllocation(_allocator.GetHandle(), _allocation, 0, size), "failed to flush buffer");
}

} // namespace VulkanRenderer
