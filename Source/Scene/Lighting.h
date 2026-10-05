#pragma once
#include "Math/Math.h"

namespace VulkanRenderer {
/** Linear RGB ambient and Lambert directional illumination; direction points toward the light. */
class Lighting {
public:
	Lighting() = default;
	virtual ~Lighting() = default;
	Lighting(const Lighting &) = delete;
	Lighting &operator=(const Lighting &) = delete;

	glm::vec3 GetDirection() const noexcept { return _direction; }

	glm::vec3 GetColor() const noexcept { return _color; }

	float GetIntensity() const noexcept { return _intensity; }

	float GetAmbient() const noexcept { return _ambient; }

	void SetDirection(const glm::vec3 &direction);
	void SetColor(const glm::vec3 &color);
	void SetIntensity(float intensity);
	void SetAmbient(float ambient);

private:
	glm::vec3 _direction{-0.45f, 0.75f, 1.0f};
	glm::vec3 _color{1.0f, 0.96f, 0.9f};
	float _intensity = 0.85f;
	float _ambient = 0.15f;
};
} // namespace VulkanRenderer
