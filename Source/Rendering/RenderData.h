#pragma once
#include <vulkan/vulkan.h>
#include <array>
#include <cstdint>
#include <cstddef>

namespace VulkanRenderer {
inline constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 3;
inline constexpr uint32_t MATERIAL_CAPACITY = 32;
inline constexpr uint32_t TEXTURE_CAPACITY = 32;

struct alignas(16) FrameUniform {
	std::array<float, 16> transform = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
	// XYZ points toward the light; W is directional intensity. Defaults preserve unlit test rendering.
	std::array<float, 4> lightDirection = {0, 0, 1, 0};
	// RGB is linear light color; W is ambient intensity.
	std::array<float, 4> lightColor = {1, 1, 1, 1};
	std::array<float, 4> cameraPosition = {0, 0, 3, 1};
	std::array<float, 16> lightTransform = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
	// X enables shadows; Y/Z are constant/slope receiver bias; W is inverse shadow resolution.
	std::array<float, 4> shadowParameters = {0, 0.0005f, 0.002f, 1.0f / 2048};
};

struct DrawData {
	VkDeviceAddress vertexAddress;
	uint32_t frameIndex;
	uint32_t materialIndex;
	std::array<float, 16> model = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
};

static_assert(sizeof(FrameUniform) == 192);
static_assert(offsetof(FrameUniform, lightDirection) == 64);
static_assert(offsetof(FrameUniform, lightColor) == 80);
static_assert(offsetof(FrameUniform, cameraPosition) == 96);
static_assert(offsetof(FrameUniform, lightTransform) == 112);
static_assert(offsetof(FrameUniform, shadowParameters) == 176);
static_assert(offsetof(DrawData, model) == 16);
static_assert(sizeof(DrawData) == 80);
static_assert(sizeof(DrawData) <= 128);
} // namespace VulkanRenderer
