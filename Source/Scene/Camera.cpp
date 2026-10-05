#include "Scene/Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace VulkanRenderer {
glm::vec3 Camera::GetForward() const {
	return glm::vec3(std::cos(_yaw) * std::cos(_pitch), std::sin(_pitch), std::sin(_yaw) * std::cos(_pitch));
}

glm::mat4 Camera::GetViewProjection(float aspectRatio) const {
	if (!std::isfinite(aspectRatio) || aspectRatio <= 0.0f) {
		throw std::invalid_argument("camera aspect ratio must be finite and positive");
	}
	const float NEAR_PLANE = _nearPlane;
	const float FAR_PLANE = _farPlane;
	const float FOCAL_LENGTH = 1.0f / std::tan(glm::radians(60.0f) * 0.5f);
	glm::mat4 projection(0.0f);
	projection[0][0] = FOCAL_LENGTH / aspectRatio;
	projection[1][1] = FOCAL_LENGTH;
	projection[2][2] = FAR_PLANE / (NEAR_PLANE - FAR_PLANE);
	projection[2][3] = -1.0f;
	projection[3][2] = NEAR_PLANE * FAR_PLANE / (NEAR_PLANE - FAR_PLANE);
	// Vulkan uses zero-to-one depth and framebuffer Y points down.
	projection[1][1] *= -1.0f;
	return projection * glm::lookAt(_position, _position + GetForward(), glm::vec3(0, 1, 0));
}

void Camera::SetClipPlanes(float nearPlane, float farPlane) {
	if (!std::isfinite(nearPlane) || !std::isfinite(farPlane) || nearPlane <= 0 || farPlane <= nearPlane) {
		throw std::invalid_argument("camera clip planes must be finite, positive, and ordered");
	}
	_nearPlane = nearPlane;
	_farPlane = farPlane;
}

void Camera::Move(const glm::vec3 &localDisplacement) {
	const auto FORWARD = GetForward();
	const auto RIGHT = glm::normalize(glm::cross(FORWARD, glm::vec3(0, 1, 0)));
	_position += RIGHT * localDisplacement.x + glm::vec3(0, 1, 0) * localDisplacement.y + FORWARD * localDisplacement.z;
}

void Camera::Look(float yawDelta, float pitchDelta) {
	_yaw = std::remainder(_yaw + yawDelta, (2.0f * glm::pi<float>()));
	_pitch = std::clamp(_pitch + pitchDelta, glm::radians(-89.0f), glm::radians(89.0f));
}
} // namespace VulkanRenderer
