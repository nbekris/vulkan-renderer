#pragma once
#include "Math/Bounds.h"
#include <array>

namespace VulkanRenderer {
/** Conservative world-space AABB rejection against Vulkan's zero-to-one clip volume. */
class Frustum {
public:
	explicit Frustum(const glm::mat4 &viewProjection);
	virtual ~Frustum() = default;
	Frustum(const Frustum &) = delete;
	Frustum &operator=(const Frustum &) = delete;
	bool IsVisible(const Bounds &bounds, const glm::mat4 &model) const;

private:
	std::array<glm::vec4, 6> _planes;
};
} // namespace VulkanRenderer
