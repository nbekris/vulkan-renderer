#include "Rendering/Memory/MemoryAllocator.h"
#include "Rendering/Core/VulkanContext.h"
#include "Rendering/Core/VulkanCheck.h"

namespace VulkanRenderer {

MemoryAllocator::MemoryAllocator(const VulkanContext &context) : _context(context) {
}

MemoryAllocator::~MemoryAllocator() noexcept {
	if (_allocator) {
		vmaDestroyAllocator(_allocator);
	}
}

VkDevice MemoryAllocator::GetDevice() const noexcept {
	return _context.GetDevice();
}

void MemoryAllocator::Initialize() {
	if (_allocator) {
		throw std::logic_error("allocator already initialized");
	}
	VmaAllocatorCreateInfo info{};
	info.instance = _context.GetInstance();
	info.physicalDevice = _context.GetPhysicalDevice();
	info.device = _context.GetDevice();
	info.vulkanApiVersion = VK_API_VERSION_1_3;
	info.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
	info.preferredLargeHeapBlockSize = 64ull * 1024 * 1024;
	CheckVulkan(vmaCreateAllocator(&info, &_allocator), "failed to create VMA allocator");
}

} // namespace VulkanRenderer
