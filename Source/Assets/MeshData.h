#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace VulkanRenderer {
/** Explicit vec4-sized fields match the shader's std430 device-address vertex layout. */
struct alignas(16) Vertex {
	std::array<float, 4> position{0, 0, 0, 1};
	std::array<float, 4> color{1, 1, 1, 1};
	std::array<float, 4> normal{0, 0, 1, 0};
	std::array<float, 4> uv{0, 0, 0, 0};
};

struct MeshData {
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
};

static_assert(sizeof(Vertex) == 64);
static_assert(offsetof(Vertex, color) == 16);
static_assert(offsetof(Vertex, normal) == 32);
static_assert(offsetof(Vertex, uv) == 48);
} // namespace VulkanRenderer
