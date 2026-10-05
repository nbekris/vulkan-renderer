#pragma once
#include "Assets/MeshData.h"
struct cgltf_primitive;

namespace VulkanRenderer {
class GltfGeometry {
public:
	GltfGeometry() = delete;
	virtual ~GltfGeometry() = default;
	GltfGeometry(const GltfGeometry &) = delete;
	GltfGeometry &operator=(const GltfGeometry &) = delete;
	static MeshData Read(const cgltf_primitive &primitive);
};
} // namespace VulkanRenderer
