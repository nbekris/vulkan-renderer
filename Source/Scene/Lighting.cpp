#include "Scene/Lighting.h"
#include <cmath>
#include <stdexcept>

namespace VulkanRenderer {
void Lighting::SetDirection(const glm::vec3 &direction) {
	const float LENGTH = glm::length(direction);
	if (!std::isfinite(LENGTH) || LENGTH < 0.000001f) {
		throw std::invalid_argument("light direction must be finite and nonzero");
	}
	_direction = direction / LENGTH;
}

void Lighting::SetColor(const glm::vec3 &color) {
	for (int i = 0; i < 3; ++i) {
		if (!std::isfinite(color[i]) || color[i] < 0) {
			throw std::invalid_argument("light color must be finite and nonnegative");
		}
	}
	_color = color;
}

void Lighting::SetIntensity(float intensity) {
	if (!std::isfinite(intensity) || intensity < 0) {
		throw std::invalid_argument("light intensity must be finite and nonnegative");
	}
	_intensity = intensity;
}

void Lighting::SetAmbient(float ambient) {
	if (!std::isfinite(ambient) || ambient < 0) {
		throw std::invalid_argument("ambient intensity must be finite and nonnegative");
	}
	_ambient = ambient;
}
} // namespace VulkanRenderer
