#pragma once
#include "VmaConfig.h"

namespace VulkanRenderer {
class VulkanContext;

/** Owns the VMA allocator; must outlive all allocated buffers. */
class MemoryAllocator {
public:
	explicit MemoryAllocator(const VulkanContext &context);
	virtual ~MemoryAllocator() noexcept;
	MemoryAllocator(const MemoryAllocator &) = delete;
	MemoryAllocator &operator=(const MemoryAllocator &) = delete;

	VmaAllocator GetHandle() const noexcept { return _allocator; }

	void Initialize();

private:
	const VulkanContext &_context;
	VmaAllocator _allocator = nullptr;
};
} // namespace VulkanRenderer
