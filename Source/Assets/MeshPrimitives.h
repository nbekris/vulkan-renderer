#pragma once
#include "Assets/MeshData.h"

namespace VulkanRenderer {
/** Produces CPU geometry without owning GPU resources. */
class MeshPrimitives {
public:
	MeshPrimitives() = delete;
	virtual ~MeshPrimitives() = default;
	MeshPrimitives(const MeshPrimitives &) = delete;
	MeshPrimitives &operator=(const MeshPrimitives &) = delete;
	static MeshData CreateTriangle();
	static MeshData CreateCube();
};
} // namespace VulkanRenderer
