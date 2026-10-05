#include "Application/DemoScene.h"
#include "Scene/Scene.h"
#include "Rendering/Resources/SceneResources.h"
#include "Rendering/Resources/Material.h"
#include "Assets/MeshPrimitives.h"
#include <vector>

namespace VulkanRenderer {
void DemoScene::Populate(SceneResources &resources, Scene &scene) {
	std::vector<uint8_t> checker(64 * 64 * 4);
	for (uint32_t y = 0; y < 64; ++y) {
		for (uint32_t x = 0; x < 64; ++x) {
			const uint8_t VALUE = ((x / 8 + y / 8) % 2) ? 255 : 70;
			const size_t PIXEL = (static_cast<size_t>(y) * 64 + x) * 4;
			checker[PIXEL] = checker[PIXEL + 1] = checker[PIXEL + 2] = VALUE;
			checker[PIXEL + 3] = 255;
		}
	}
	MaterialData textured;
	textured.textureIndex = resources.AddTexture({64, 64}, checker);
	const uint32_t FIRST_MATERIAL = resources.AddMaterial(textured);
	textured.tint = {0.55f, 0.8f, 1.0f, 1.0f};
	const uint32_t SECOND_MATERIAL = resources.AddMaterial(textured);
	const auto cube = resources.CreateMesh(MeshPrimitives::CreateCube());
	auto &first = scene.AddObject(cube, FIRST_MATERIAL).GetTransform();
	first.SetPosition(glm::vec3(-0.7f, 0, 0));
	first.SetRotation(glm::vec3(0.3f, 0.5f, 0));
	auto &second = scene.AddObject(cube, SECOND_MATERIAL).GetTransform();
	second.SetPosition(glm::vec3(0.7f, 0, -0.6f));
	second.SetRotation(glm::vec3(-0.2f, -0.5f, 0.25f));
	second.SetScale(glm::vec3(0.8f));
}
} // namespace VulkanRenderer
