#include "Unit/FrustumTests.h"
#include "Math/Frustum.h"
#include "Scene/Camera.h"
#include <stdexcept>
#include <glm/gtc/matrix_transform.hpp>

namespace VulkanRenderer {
namespace {
void Require(bool condition, const char *message) {
	if (!condition) {
		throw std::runtime_error(message);
	}
}
} // namespace

void RunFrustumTests() {
	Camera camera;
	const Bounds BOX{glm::vec3(-0.1f), glm::vec3(0.1f)};
	const auto MODEL = [](float x, float y, float z) { return glm::translate(glm::mat4(1), glm::vec3(x, y, z)); };
	const Frustum FRUSTUM(camera.GetViewProjection(1));
	Require(FRUSTUM.IsVisible(BOX, MODEL(0, 0, 0)), "central object was culled");
	for (const auto &point : {glm::vec3(-10, 0, 0), glm::vec3(10, 0, 0), glm::vec3(0, -10, 0), glm::vec3(0, 10, 0),
							  glm::vec3(0, 0, 4), glm::vec3(0, 0, -110)}) {
		Require(!FRUSTUM.IsVisible(BOX, MODEL(point.x, point.y, point.z)), "object beyond frustum plane was accepted");
	}
	Require(FRUSTUM.IsVisible(BOX, MODEL(0, 0, 2.8f)), "near-plane object was incorrectly culled");
	Require(!FRUSTUM.IsVisible(Bounds{glm::vec3(0), glm::vec3(0)}, MODEL(0, 0, 2.95f)),
			"Vulkan zero-to-one near plane failed");
	Require(
		FRUSTUM.IsVisible(BOX, glm::scale(glm::rotate(glm::mat4(1), 0.6f, glm::vec3(0, 1, 0)), glm::vec3(-2, 3, 1))),
		"rotated, mirrored, or nonuniform bounds were culled");
	const Frustum CLIP(glm::mat4(1));
	Require(CLIP.IsVisible(BOX, MODEL(1.1f, 0, .5f)), "touching plane must remain visible");
	glm::mat4 shear(1);
	shear[1][0] = 4;
	Require(CLIP.IsVisible(BOX, shear), "sheared bounds were incorrectly culled");
	Require(Frustum(camera.GetViewProjection(2)).IsVisible(BOX, MODEL(2.5f, 0, 0))
				&& !FRUSTUM.IsVisible(BOX, MODEL(2.5f, 0, 0)),
			"aspect ratio did not update horizontal culling");
	camera.Move(glm::vec3(10, 0, 0));
	Require(!Frustum(camera.GetViewProjection(1)).IsVisible(BOX, MODEL(0, 0, 0)),
			"camera movement did not update culling");
}
} // namespace VulkanRenderer
