#pragma once
#include "AllocatedBuffer.h"
#include <array>

namespace VulkanRenderer {
struct Vertex {
	float position[2];
	float color[3];
};

/** Owns triangle vertex data and describes its vertex layout. */
class TriangleMesh {
public:
	static VkVertexInputBindingDescription GetBinding();
	static std::array<VkVertexInputAttributeDescription, 2> GetAttributes();
	explicit TriangleMesh(const MemoryAllocator &allocator);
	virtual ~TriangleMesh() = default;
	TriangleMesh(const TriangleMesh &) = delete;
	TriangleMesh &operator=(const TriangleMesh &) = delete;
	void Record(VkCommandBuffer commandBuffer) const;
	void Initialize();

private:
	AllocatedBuffer _vertexBuffer;
};
} // namespace VulkanRenderer
