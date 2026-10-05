#pragma once

namespace VulkanRenderer {
class VulkanContext;
class MemoryAllocator;
class SwapChain;
void RunAssimpTests(const VulkanContext &context, const MemoryAllocator &allocator, const SwapChain &swapChain);
} // namespace VulkanRenderer
