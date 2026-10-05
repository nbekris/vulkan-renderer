#include "Rendering/Memory/AllocatedBuffer.h"
#include "Rendering/Memory/MemoryAllocator.h"
#include "Rendering/Core/VulkanCheck.h"
#include <cstring>

namespace VulkanRenderer {

AllocatedBuffer::AllocatedBuffer(const MemoryAllocator &allocator) : _allocator(allocator) {
}

AllocatedBuffer::~AllocatedBuffer() noexcept {
	if (_buffer) {
		vmaDestroyBuffer(_allocator.GetHandle(), _buffer, _allocation);
	}
}

void AllocatedBuffer::Initialize(VkDeviceSize size, VkBufferUsageFlags usage, BufferMemory memory) {
	if (_buffer) {
		throw std::logic_error("buffer already initialized");
	}
	if (size == 0) {
		throw std::invalid_argument("buffer size must be nonzero");
	}
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	VmaAllocationCreateInfo allocationInfo{};
	allocationInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
	allocationInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	if (memory == BufferMemory::HostWrite) {
		allocationInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
		allocationInfo.requiredFlags = 0;
		allocationInfo.flags
			= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
	}
	VmaAllocationInfo mapped{};
	CheckVulkan(vmaCreateBuffer(_allocator.GetHandle(), &bufferInfo, &allocationInfo, &_buffer, &_allocation, &mapped),
				"failed to allocate buffer");
	_mappedData = mapped.pMappedData;
	_size = size;
	_usage = usage;
}

VkDeviceAddress AllocatedBuffer::GetDeviceAddress() const {
	if (!_buffer || !(_usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)) {
		throw std::logic_error("buffer was not created for device addressing");
	}
	VkBufferDeviceAddressInfo info{};
	info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
	info.buffer = _buffer;
	return vkGetBufferDeviceAddress(_allocator.GetDevice(), &info);
}

void AllocatedBuffer::Upload(const void *data, VkDeviceSize size) {
	if (!_mappedData || !data || size == 0 || size > _size) {
		throw std::invalid_argument("invalid buffer upload");
	}
	std::memcpy(_mappedData, data, static_cast<size_t>(size));
	CheckVulkan(vmaFlushAllocation(_allocator.GetHandle(), _allocation, 0, size), "failed to flush buffer");
}

} // namespace VulkanRenderer
