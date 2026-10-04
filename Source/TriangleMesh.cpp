#include "TriangleMesh.h"
#include <cstddef>

namespace VulkanRenderer {

namespace {
constexpr Vertex VERTICES[]
	= {{{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}}, {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}}, {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}};
}

TriangleMesh::TriangleMesh(const MemoryAllocator &allocator) : _vertexBuffer(allocator) {
}

VkVertexInputBindingDescription TriangleMesh::GetBinding() {
	return {0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX};
}

std::array<VkVertexInputAttributeDescription, 2> TriangleMesh::GetAttributes() {
	return {{{0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, position)},
			 {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color)}}};
}

void TriangleMesh::Initialize() {
	_vertexBuffer.Initialize(sizeof(VERTICES), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
	_vertexBuffer.Upload(VERTICES, sizeof(VERTICES));
}

void TriangleMesh::Record(VkCommandBuffer commandBuffer) const {
	const VkBuffer BUFFER = _vertexBuffer.GetHandle();
	const VkDeviceSize OFFSET = 0;
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &BUFFER, &OFFSET);
	vkCmdDraw(commandBuffer, static_cast<uint32_t>(std::size(VERTICES)), 1, 0, 0);
}

} // namespace VulkanRenderer
