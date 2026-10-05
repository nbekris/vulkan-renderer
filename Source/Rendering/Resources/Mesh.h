#pragma once
#include "Rendering/Memory/AllocatedBuffer.h"
#include "Assets/MeshData.h"
#include "Math/Bounds.h"
#include <span>

namespace VulkanRenderer {
class VulkanContext;

/** Owns arbitrary indexed triangle geometry; vertex data is pulled using its device address. */
class Mesh {
public:
	Mesh(const VulkanContext &context, const MemoryAllocator &allocator);
	virtual ~Mesh() = default;
	Mesh(const Mesh &) = delete;
	Mesh &operator=(const Mesh &) = delete;

	bool IsInitialized() const noexcept { return _indexCount != 0; }

	uint32_t GetIndexCount() const noexcept { return _indexCount; }

	const Bounds &GetBounds() const noexcept { return _bounds; }

	VkDeviceAddress GetVertexAddress() const;
	void Record(VkCommandBuffer commandBuffer) const;
	void Initialize(std::span<const Vertex> vertices, std::span<const uint32_t> indices);

private:
	const VulkanContext &_context;
	const MemoryAllocator &_allocator;
	AllocatedBuffer _vertexBuffer;
	AllocatedBuffer _indexBuffer;
	uint32_t _indexCount = 0;
	Bounds _bounds;
};
} // namespace VulkanRenderer
