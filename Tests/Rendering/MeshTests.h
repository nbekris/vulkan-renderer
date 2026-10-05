#pragma once

namespace VulkanRenderer {
class VulkanContext;
class MemoryAllocator;
class SwapChain;
void RunMeshTests(const VulkanContext &context, const MemoryAllocator &allocator, const SwapChain &swapChain);
} // namespace VulkanRenderer
