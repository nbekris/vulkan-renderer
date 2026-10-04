#include "MemoryAllocator.h"
#include "VulkanContext.h"
#include "VulkanCheck.h"

namespace VulkanRenderer {

MemoryAllocator::MemoryAllocator(const VulkanContext &context) : _context(context) {
}

MemoryAllocator::~MemoryAllocator() noexcept {
	if (_allocator) {
		vmaDestroyAllocator(_allocator);
	}
}

void MemoryAllocator::Initialize() {
	if (_allocator) {
		throw std::logic_error("allocator already initialized");
	}
	VmaAllocatorCreateInfo info{};
	info.instance = _context.GetInstance();
	info.physicalDevice = _context.GetPhysicalDevice();
	info.device = _context.GetDevice();
	info.vulkanApiVersion = VK_API_VERSION_1_0;
	CheckVulkan(vmaCreateAllocator(&info, &_allocator), "failed to create VMA allocator");
}

} // namespace VulkanRenderer
