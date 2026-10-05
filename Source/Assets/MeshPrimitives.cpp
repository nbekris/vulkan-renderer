#include "Assets/MeshPrimitives.h"
#include "Math/Math.h"

namespace VulkanRenderer {
MeshData MeshPrimitives::CreateTriangle() {
	MeshData data;
	data.vertices = {{{0, -0.5f, 0, 1}, {1, 0, 0, 1}, {0, 0, 1, 0}, {0.5f, 0, 0, 0}},
					 {{0.5f, 0.5f, 0, 1}, {0, 1, 0, 1}, {0, 0, 1, 0}, {1, 1, 0, 0}},
					 {{-0.5f, 0.5f, 0, 1}, {0, 0, 1, 1}, {0, 0, 1, 0}, {0, 1, 0, 0}}};
	data.indices = {0, 1, 2};
	return data;
}

MeshData MeshPrimitives::CreateCube() {
	MeshData data;
	// Four vertices per face preserve hard normals and independent face UVs.
	const glm::vec3 NORMALS[] = {{0, 0, 1}, {0, 0, -1}, {1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}};
	const glm::vec3 RIGHT[] = {{1, 0, 0}, {-1, 0, 0}, {0, 0, -1}, {0, 0, 1}, {1, 0, 0}, {1, 0, 0}};
	const glm::vec3 UP[] = {{0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {0, 0, -1}, {0, 0, 1}};
	const glm::vec2 UVS[] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
	const std::array<float, 4> COLORS[] = {{0.9f, 0.4f, 0.25f, 1}, {0.3f, 0.6f, 0.9f, 1}, {0.9f, 0.7f, 0.3f, 1},
										   {0.5f, 0.3f, 0.8f, 1},  {0.4f, 0.8f, 0.6f, 1}, {0.7f, 0.5f, 0.4f, 1}};
	for (uint32_t face = 0; face < 6; ++face) {
		const uint32_t BASE = static_cast<uint32_t>(data.vertices.size());
		for (const auto &UV : UVS) {
			const auto POSITION = 0.5f * NORMALS[face] + (UV.x - 0.5f) * RIGHT[face] + (UV.y - 0.5f) * UP[face];
			data.vertices.push_back({{POSITION.x, POSITION.y, POSITION.z, 1},
									 COLORS[face],
									 {NORMALS[face].x, NORMALS[face].y, NORMALS[face].z, 0},
									 {UV.x, UV.y, 0, 0}});
		}
		data.indices.insert(data.indices.end(), {BASE, BASE + 1, BASE + 2, BASE, BASE + 2, BASE + 3});
	}
	return data;
}
} // namespace VulkanRenderer
