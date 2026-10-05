#pragma once
#include "Math/Math.h"

namespace VulkanRenderer {
struct Bounds {
	glm::vec3 minimum{0};
	glm::vec3 maximum{0};
};
} // namespace VulkanRenderer
