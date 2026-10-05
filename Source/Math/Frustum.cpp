#include "Math/Frustum.h"

namespace VulkanRenderer {
Frustum::Frustum(const glm::mat4 &viewProjection) {
	const auto ROW = [&viewProjection](int i) {
		return glm::vec4(viewProjection[0][i], viewProjection[1][i], viewProjection[2][i], viewProjection[3][i]);
	};
	_planes = {ROW(3) + ROW(0), ROW(3) - ROW(0), ROW(3) + ROW(1), ROW(3) - ROW(1), ROW(2), ROW(3) - ROW(2)};
}

bool Frustum::IsVisible(const Bounds &bounds, const glm::mat4 &model) const {
	const auto LOCAL_CENTER = (bounds.minimum + bounds.maximum) * 0.5f;
	const auto LOCAL_EXTENT = (bounds.maximum - bounds.minimum) * 0.5f;
	const auto CENTER = glm::vec3(model * glm::vec4(LOCAL_CENTER, 1));
	const auto EXTENT = glm::abs(glm::vec3(model[0])) * LOCAL_EXTENT.x + glm::abs(glm::vec3(model[1])) * LOCAL_EXTENT.y
						+ glm::abs(glm::vec3(model[2])) * LOCAL_EXTENT.z;
	for (const auto &PLANE : _planes) {
		const auto NORMAL = glm::vec3(PLANE);
		const float RADIUS = glm::dot(glm::abs(NORMAL), EXTENT);
		if (glm::dot(NORMAL, CENTER) + PLANE.w + RADIUS < -0.00001f * glm::length(NORMAL)) {
			return false;
		}
	}
	return true;
}
} // namespace VulkanRenderer
