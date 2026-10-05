#include "Math/Transform.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <stdexcept>

namespace VulkanRenderer {
void Transform::SetScale(const glm::vec3 &scale) {
	for (int i = 0; i < 3; ++i) {
		if (!std::isfinite(scale[i]) || std::abs(scale[i]) < 0.000001f) {
			throw std::invalid_argument("scale must be finite and nonzero for normal transformation");
		}
	}
	_scale = scale;
}

glm::mat4 Transform::GetMatrix() const {
	auto matrix = glm::translate(glm::mat4(1.0f), _position);
	matrix = glm::rotate(matrix, _rotation.z, glm::vec3(0, 0, 1));
	matrix = glm::rotate(matrix, _rotation.y, glm::vec3(0, 1, 0));
	matrix = glm::rotate(matrix, _rotation.x, glm::vec3(1, 0, 0));
	return glm::scale(matrix, _scale);
}
} // namespace VulkanRenderer
