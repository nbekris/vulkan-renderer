#include "Unit/CameraTransformTests.h"
#include "Scene/Camera.h"
#include "Math/Transform.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <stdexcept>

namespace VulkanRenderer {
namespace {
void Require(bool condition, const char *message) {
	if (!condition) {
		throw std::runtime_error(message);
	}
}
} // namespace

void RunCameraTransformTests() {
	Transform object;
	object.SetPosition(glm::vec3(2, 3, 4));
	object.SetScale(glm::vec3(2, 3, 4));
	object.SetRotation(glm::vec3(0, 0, glm::half_pi<float>()));
	const auto POINT = object.GetMatrix() * glm::vec4(1, 0, 0, 1);
	Require(glm::length(POINT - glm::vec4(2, 5, 4, 1)) < 0.0001f, "incorrect transform composition");
	Camera camera;
	const auto INITIAL = camera.GetPosition();
	camera.Move(glm::vec3(1, 0, 2));
	Require(glm::length(camera.GetPosition() - INITIAL - glm::vec3(1, 0, -2)) < 0.0001f,
			"camera movement is not relative to its orientation");
	camera.SetPosition(glm::vec3(0));
	const auto PROJECT = camera.GetViewProjection(1.0f);
	const auto NEAR = PROJECT * glm::vec4(0, 0, -0.1f, 1);
	const auto FAR = PROJECT * glm::vec4(0, 0, -100.0f, 1);
	Require(std::abs(NEAR.z / NEAR.w) < 0.0001f && std::abs(FAR.z / FAR.w - 1) < 0.0001f,
			"projection does not use Vulkan zero-to-one depth");
	Require(PROJECT[1][1] < 0, "projection does not invert framebuffer Y");
	Require(std::abs(camera.GetViewProjection(2.0f)[0][0] * 2 - PROJECT[0][0]) < 0.0001f,
			"projection does not respond to resizing aspect ratio");
	camera.Look(glm::half_pi<float>(), 100);
	Require(std::isfinite(camera.GetViewProjection(1.0f)[0][0]), "pitch clamp produced a singular view");
	bool rejected = false;
	try {
		camera.GetViewProjection(0);
	} catch (const std::invalid_argument &) {
		rejected = true;
	}
	Require(rejected, "zero aspect ratio must be rejected");
	rejected = false;
	try {
		object.SetScale(glm::vec3(1, 0, 1));
	} catch (const std::invalid_argument &) {
		rejected = true;
	}
	Require(rejected, "singular scale must be rejected before normal transformation");
}
} // namespace VulkanRenderer
