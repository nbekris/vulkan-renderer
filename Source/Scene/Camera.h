#pragma once
#include "Math/Math.h"

namespace VulkanRenderer {
/** Right-handed perspective camera; forward is initially negative Z. */
class Camera {
public:
	Camera() = default;
	virtual ~Camera() = default;
	Camera(const Camera &) = delete;
	Camera &operator=(const Camera &) = delete;

	const glm::vec3 &GetPosition() const noexcept { return _position; }

	void SetPosition(const glm::vec3 &position) noexcept { _position = position; }

	glm::vec3 GetForward() const;
	glm::mat4 GetViewProjection(float aspectRatio) const;
	void SetClipPlanes(float nearPlane, float farPlane);
	void Move(const glm::vec3 &localDisplacement);
	void Look(float yawDelta, float pitchDelta);

private:
	glm::vec3 _position{0, 0, 3};
	float _yaw = -1.570796327f;
	float _pitch = 0.0f;
	float _nearPlane = 0.1f;
	float _farPlane = 100.0f;
};
} // namespace VulkanRenderer
