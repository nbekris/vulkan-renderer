#include "Scene/CameraController.h"
#include "Scene/Camera.h"
#include "Platform/Window.h"
#include <GLFW/glfw3.h>
#include <algorithm>

namespace VulkanRenderer {
void CameraController::Update(const Window &window, Camera &camera) {
	const double NOW = glfwGetTime();
	const float DELTA = _initialized ? static_cast<float>(std::clamp(NOW - _lastTime, 0.0, 0.1)) : 0.0f;
	_initialized = true;
	_lastTime = NOW;
	auto *handle = window.GetHandle();
	const bool FOCUSED
		= glfwGetWindowAttrib(handle, GLFW_FOCUSED) == GLFW_TRUE && window.IsRenderable() && !window.ShouldClose();
	const bool LOOKING = FOCUSED && glfwGetMouseButton(handle, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
	if (LOOKING != _looking) {
		glfwSetInputMode(handle, GLFW_CURSOR, LOOKING ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
		glfwGetCursorPos(handle, &_lastX, &_lastY);
	}
	double x = 0;
	double y = 0;
	glfwGetCursorPos(handle, &x, &y);
	if (LOOKING && _looking) {
		camera.Look(static_cast<float>(x - _lastX) * 0.0025f, static_cast<float>(_lastY - y) * 0.0025f);
	}
	_lastX = x;
	_lastY = y;
	_looking = LOOKING;
	if (!FOCUSED) {
		return;
	}
	const auto PRESSED = [handle](int key) { return glfwGetKey(handle, key) == GLFW_PRESS ? 1.0f : 0.0f; };
	glm::vec3 movement(PRESSED(GLFW_KEY_D) - PRESSED(GLFW_KEY_A), PRESSED(GLFW_KEY_E) - PRESSED(GLFW_KEY_Q),
					   PRESSED(GLFW_KEY_W) - PRESSED(GLFW_KEY_S));
	if (glm::length(movement) > 0.0f) {
		movement = glm::normalize(movement);
		camera.Move(movement * DELTA * (PRESSED(GLFW_KEY_LEFT_SHIFT) > 0.0f ? 6.0f : 2.0f));
	}
}
} // namespace VulkanRenderer
