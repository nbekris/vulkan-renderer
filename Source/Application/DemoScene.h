#pragma once

namespace VulkanRenderer {
class Scene;
class SceneResources;

class DemoScene {
public:
	DemoScene() = delete;
	virtual ~DemoScene() = default;
	DemoScene(const DemoScene &) = delete;
	DemoScene &operator=(const DemoScene &) = delete;
	static void Populate(SceneResources &resources, Scene &scene);
};
} // namespace VulkanRenderer
