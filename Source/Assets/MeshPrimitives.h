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
	/** Creates a unit XZ plane centered at the origin, with upward normals and unit UVs. */
	static MeshData CreatePlane();
};
} // namespace VulkanRenderer
