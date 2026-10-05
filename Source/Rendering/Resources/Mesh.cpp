#include "Rendering/Resources/Mesh.h"
#include "Rendering/Memory/ResourceUploader.h"
#include <limits>
#include <stdexcept>
#include <cmath>

namespace VulkanRenderer {
Mesh::Mesh(const VulkanContext &context, const MemoryAllocator &allocator)
	: _context(context), _allocator(allocator), _vertexBuffer(allocator), _indexBuffer(allocator) {
}

void Mesh::Initialize(std::span<const Vertex> vertices, std::span<const uint32_t> indices) {
	if (_vertexBuffer.GetHandle() || _indexBuffer.GetHandle()) {
		throw std::logic_error("mesh already initialized");
	}
	if (vertices.empty() || indices.empty() || indices.size() % 3 != 0
		|| vertices.size() > std::numeric_limits<uint32_t>::max()
		|| indices.size() > std::numeric_limits<uint32_t>::max()) {
		throw std::invalid_argument("mesh requires vertices and complete triangle indices within 32-bit limits");
	}
	for (auto index : indices) {
		if (index >= vertices.size()) {
			throw std::out_of_range("mesh index is outside the vertex array");
		}
	}
	Bounds bounds{glm::vec3(std::numeric_limits<float>::max()), glm::vec3(std::numeric_limits<float>::lowest())};
	for (const auto &vertex : vertices) {
		for (auto value : vertex.position) {
			if (!std::isfinite(value)) {
				throw std::invalid_argument("mesh positions must be finite");
			}
		}
		if (vertex.position[3] != 1.0f) {
			throw std::invalid_argument("mesh position W must be one");
		}
		const glm::vec3 POSITION(vertex.position[0], vertex.position[1], vertex.position[2]);
		bounds.minimum = glm::min(bounds.minimum, POSITION);
		bounds.maximum = glm::max(bounds.maximum, POSITION);
	}
	_vertexBuffer.Initialize(vertices.size_bytes(),
							 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
								 | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
							 BufferMemory::DeviceLocal);
	_indexBuffer.Initialize(indices.size_bytes(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
							BufferMemory::DeviceLocal);
	ResourceUploader uploader(_context, _allocator);
	uploader.Initialize();
	uploader.Upload(_vertexBuffer, vertices.data(), vertices.size_bytes(), VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,
					VK_ACCESS_2_SHADER_STORAGE_READ_BIT);
	uploader.Upload(_indexBuffer, indices.data(), indices.size_bytes(), VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT,
					VK_ACCESS_2_INDEX_READ_BIT);
	_indexCount = static_cast<uint32_t>(indices.size());
	_bounds = bounds;
}

VkDeviceAddress Mesh::GetVertexAddress() const {
	if (!IsInitialized()) {
		throw std::logic_error("mesh is not initialized");
	}
	return _vertexBuffer.GetDeviceAddress();
}

void Mesh::Record(VkCommandBuffer commandBuffer) const {
	if (!IsInitialized()) {
		throw std::logic_error("mesh is not initialized");
	}
	vkCmdBindIndexBuffer(commandBuffer, _indexBuffer.GetHandle(), 0, VK_INDEX_TYPE_UINT32);
	vkCmdDrawIndexed(commandBuffer, _indexCount, 1, 0, 0, 0);
}
} // namespace VulkanRenderer
