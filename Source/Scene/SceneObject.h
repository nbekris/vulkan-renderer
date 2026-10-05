#pragma once
#include "Math/Transform.h"
#include <cstdint>
#include <memory>

namespace VulkanRenderer {
class Mesh;

/** Composes shared geometry, an object transform, and an index into the material table. */
class SceneObject {
public:
	SceneObject(std::shared_ptr<const Mesh> mesh, uint32_t materialIndex);
	virtual ~SceneObject() = default;
	SceneObject(const SceneObject &) = delete;
	SceneObject &operator=(const SceneObject &) = delete;

	const Mesh &GetMesh() const noexcept { return *_mesh; }

	const Transform &GetTransform() const noexcept { return _transform; }

	Transform &GetTransform() noexcept { return _transform; }

	uint32_t GetMaterialIndex() const noexcept { return _materialIndex; }

	glm::mat4 GetModelMatrix() const { return _nodeTransform * _transform.GetMatrix(); }

	void SetNodeTransform(const glm::mat4 &matrix);

	void SetMaterialIndex(uint32_t materialIndex);

private:
	std::shared_ptr<const Mesh> _mesh;
	Transform _transform;
	glm::mat4 _nodeTransform{1};
	uint32_t _materialIndex = 0;
};
} // namespace VulkanRenderer
