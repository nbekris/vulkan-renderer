#pragma once
#include "Math/Math.h"

namespace VulkanRenderer {
/** Object-local translation, Euler rotation in radians, and scale. */
class Transform {
public:
	Transform() = default;
	virtual ~Transform() = default;
	Transform(const Transform &) = delete;
	Transform &operator=(const Transform &) = delete;

	const glm::vec3 &GetPosition() const noexcept { return _position; }

	void SetPosition(const glm::vec3 &position) noexcept { _position = position; }

	void SetRotation(const glm::vec3 &rotation) noexcept { _rotation = rotation; }

	void SetScale(const glm::vec3 &scale);

	glm::mat4 GetMatrix() const;

private:
	glm::vec3 _position{0.0f};
	glm::vec3 _rotation{0.0f};
	glm::vec3 _scale{1.0f};
};
} // namespace VulkanRenderer
