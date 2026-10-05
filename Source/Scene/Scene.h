#pragma once
#include "Scene/Camera.h"
#include "Scene/Lighting.h"
#include "Scene/SceneObject.h"
#include "Rendering/RenderData.h"
#include <memory>
#include <vector>

namespace VulkanRenderer {
/** Persistent scene objects and camera, independent of swapchain generations. */
class Scene {
public:
	Scene() = default;
	virtual ~Scene() = default;
	Scene(const Scene &) = delete;
	Scene &operator=(const Scene &) = delete;

	Camera &GetCamera() noexcept { return _camera; }

	Lighting &GetLighting() noexcept { return _lighting; }

	const Lighting &GetLighting() const noexcept { return _lighting; }

	const std::vector<std::unique_ptr<SceneObject>> &GetObjects() const noexcept { return _objects; }

	SceneObject &GetObject(size_t index) { return *_objects.at(index); }

	const SceneObject &GetObject(size_t index) const { return *_objects.at(index); }

	FrameUniform GetFrameUniform(float aspectRatio) const;
	SceneObject &AddObject(std::shared_ptr<const Mesh> mesh, uint32_t materialIndex = 0,
						   const glm::mat4 &nodeTransform = glm::mat4(1));
	void TruncateObjects(size_t count);

private:
	Camera _camera;
	Lighting _lighting;
	std::vector<std::unique_ptr<SceneObject>> _objects;
};
} // namespace VulkanRenderer
