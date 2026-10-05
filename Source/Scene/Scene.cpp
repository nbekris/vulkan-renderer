#include "Scene/Scene.h"
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <stdexcept>

namespace VulkanRenderer {
SceneObject &Scene::AddObject(std::shared_ptr<const Mesh> mesh, uint32_t materialIndex,
							  const glm::mat4 &nodeTransform) {
	auto object = std::make_unique<SceneObject>(std::move(mesh), materialIndex);
	object->SetNodeTransform(nodeTransform);
	_objects.push_back(std::move(object));
	return *_objects.back();
}

void Scene::TruncateObjects(size_t count) {
	if (count > _objects.size()) {
		throw std::out_of_range("invalid scene object checkpoint");
	}
	_objects.resize(count);
}

FrameUniform Scene::GetFrameUniform(float aspectRatio) const {
	FrameUniform uniform;
	const auto MATRIX = _camera.GetViewProjection(aspectRatio);
	std::copy_n(glm::value_ptr(MATRIX), uniform.transform.size(), uniform.transform.begin());
	const auto DIRECTION = _lighting.GetDirection();
	const auto COLOR = _lighting.GetColor();
	uniform.lightDirection = {DIRECTION.x, DIRECTION.y, DIRECTION.z, _lighting.GetIntensity()};
	uniform.lightColor = {COLOR.x, COLOR.y, COLOR.z, _lighting.GetAmbient()};
	return uniform;
}
} // namespace VulkanRenderer
