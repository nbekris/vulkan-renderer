#include "TrianglePass.h"
#include "RenderTargets.h"
#include "SwapChain.h"

namespace VulkanRenderer {

TrianglePass::TrianglePass(const VulkanContext &context, const MemoryAllocator &allocator)
	: _pipeline(context), _mesh(allocator) {
}

void TrianglePass::Initialize(const RenderTargets &targets, const SwapChain &swapChain) {
	const auto BINDING = TriangleMesh::GetBinding();
	const auto ATTRIBUTES = TriangleMesh::GetAttributes();
	_mesh.Initialize();
	_pipeline.Initialize(targets.GetRenderPass(), swapChain.GetExtent(), BINDING, ATTRIBUTES, "Shaders/vert.spv",
						 "Shaders/frag.spv");
}

void TrianglePass::Record(VkCommandBuffer commandBuffer) const {
	_pipeline.Bind(commandBuffer);
	_mesh.Record(commandBuffer);
}

} // namespace VulkanRenderer
