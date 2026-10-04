#pragma once
#include "IDrawCommands.h"
#include "GraphicsPipeline.h"
#include "TriangleMesh.h"

namespace VulkanRenderer {
class RenderTargets;
class SwapChain;

/** Composes the triangle's pipeline and mesh behind a narrow drawing interface. */
class TrianglePass : public IDrawCommands {
public:
	TrianglePass(const VulkanContext &context, const MemoryAllocator &allocator);
	~TrianglePass() override = default;
	TrianglePass(const TrianglePass &) = delete;
	TrianglePass &operator=(const TrianglePass &) = delete;
	void Record(VkCommandBuffer commandBuffer) const override;
	void Initialize(const RenderTargets &targets, const SwapChain &swapChain);

private:
	GraphicsPipeline _pipeline;
	TriangleMesh _mesh;
};
} // namespace VulkanRenderer
