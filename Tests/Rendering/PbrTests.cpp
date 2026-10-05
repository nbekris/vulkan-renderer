#include "Rendering/PbrTests.h"
#include "Support/RenderProbe.h"
#include "Rendering/Resources/SceneResources.h"
#include "Rendering/Resources/Material.h"
#include "Assets/MeshPrimitives.h"
#include "Rendering/MeshRenderer.h"
#include "Rendering/Frames/FrameRing.h"
#include "Rendering/Descriptors/GlobalDescriptors.h"
#include "Scene/Scene.h"
#include <cmath>
#include <limits>

namespace VulkanRenderer {
namespace {
void Require(bool condition, const char *message) {
	if (!condition) {
		throw std::runtime_error(message);
	}
}

int OutputByte(float linear, VkFormat format) {
	if (format == VK_FORMAT_B8G8R8A8_SRGB || format == VK_FORMAT_R8G8B8A8_SRGB) {
		linear = linear <= .0031308f ? 12.92f * linear : 1.055f * std::pow(linear, 1.0f / 2.4f) - .055f;
	}
	return static_cast<int>(std::lround(linear * 255));
}
} // namespace

void RunPbrTests(const VulkanContext &context, const MemoryAllocator &allocator, const SwapChain &swapChain) {
	SceneResources resources(context, allocator);
	resources.Initialize();
	for (const auto VALUE : {-0.1f, 1.1f, std::numeric_limits<float>::quiet_NaN()}) {
		for (bool metallic : {false, true}) {
			MaterialData invalid;
			(metallic ? invalid.metallic : invalid.roughness) = VALUE;
			bool rejected = false;
			try {
				resources.AddMaterial(invalid);
			} catch (const std::invalid_argument &) {
				rejected = true;
			}
			Require(rejected, "invalid PBR material parameter was accepted");
		}
	}
	MaterialData material;
	material.roughness = 1;
	const auto DIELECTRIC = resources.AddMaterial(material);
	material.tint = {.8f, 0, 0, 1};
	const auto RED_DIELECTRIC = resources.AddMaterial(material);
	material.metallic = 1;
	const auto ROUGH_METAL = resources.AddMaterial(material);
	material.roughness = .5f;
	const auto SMOOTH_METAL = resources.AddMaterial(material);
	material.roughness = 0;
	const auto ZERO_ROUGHNESS = resources.AddMaterial(material);
	material.metallic = 0;
	material.roughness = 1;
	material.unlit = 1;
	const auto UNLIT = resources.AddMaterial(material);
	auto geometry = MeshPrimitives::CreateTriangle();
	for (auto &vertex : geometry.vertices) {
		vertex.color = {1, 1, 1, 1};
	}
	const auto MESH = resources.CreateMesh(geometry);
	resources.Freeze();
	Scene scene;
	scene.AddObject(MESH, DIELECTRIC);
	FrameRing frames(context, allocator);
	frames.Initialize(3);
	GlobalDescriptors descriptors(context, resources, frames);
	descriptors.Initialize();
	MeshRenderer renderer(context, descriptors);
	renderer.Initialize(swapChain);
	renderer.SetScene(scene);
	RenderProbe probe(context, allocator, swapChain);
	probe.Initialize();
	FrameUniform uniform;
	uniform.lightDirection = {0, 0, 1, 1};
	uniform.lightColor = {1, 1, 1, 0};
	const auto RENDER = [&](uint32_t index) {
		auto &frame = frames.GetFrame(index);
		frame.Wait();
		frame.Prepare(uniform);
		probe.Render(renderer, frame, index);
		return probe.GetCenterRgba();
	};
	// Closed-form normal incidence: (1-F0)/pi + F0/(4*pi), when roughness=1.
	const int DIELECTRIC_REFERENCE = OutputByte(.30876059f, swapChain.GetImageFormat());
	for (uint32_t i = 0; i < 3; ++i) {
		const auto PIXEL = RENDER(i);
		Require(std::abs(static_cast<int>(PIXEL[0]) - DIELECTRIC_REFERENCE) <= 3 && PIXEL[0] == PIXEL[1]
					&& PIXEL[1] == PIXEL[2],
				"dielectric BRDF reference or frame layout is incorrect");
	}
	scene.GetObject(0).SetMaterialIndex(RED_DIELECTRIC);
	const auto RED = RENDER(0);
	Require(RED[0] > RED[1] && RED[1] > 3 && RED[2] > 3, "dielectric specular must retain an achromatic component");
	scene.GetObject(0).SetMaterialIndex(ROUGH_METAL);
	const auto METAL = RENDER(0);
	Require(std::abs(static_cast<int>(METAL[0]) - OutputByte(.06366198f, swapChain.GetImageFormat())) <= 3
				&& METAL[1] < 3 && METAL[2] < 3,
			"metal specular tint or diffuse suppression is incorrect");
	scene.GetObject(0).SetMaterialIndex(SMOOTH_METAL);
	const auto SMOOTH = RENDER(0);
	Require(SMOOTH[0] > METAL[0] + 60, "roughness did not change specular peak intensity");
	uniform.cameraPosition = {2, 0, 3, 1};
	Require(RENDER(0)[0] < SMOOTH[0] - 40, "specular reflection did not respond to camera position");
	uniform.cameraPosition = {0, 0, -3, 1};
	Require(RENDER(0)[0] < 3, "back-facing view produced direct lighting");
	uniform.cameraPosition = {0, 0, 3, 1};
	uniform.lightDirection = {0, 0, -1, 1};
	Require(RENDER(0)[0] < 3, "back-facing light produced direct lighting");
	uniform.lightDirection = {0, 0, 1, 1};
	scene.GetObject(0).SetMaterialIndex(ZERO_ROUGHNESS);
	Require(RENDER(0)[0] == 255, "zero roughness did not produce a stable clamped specular highlight");
	uniform.lightDirection[3] = 0;
	scene.GetObject(0).SetMaterialIndex(UNLIT);
	Require(std::abs(static_cast<int>(RENDER(0)[0]) - OutputByte(.8f, swapChain.GetImageFormat())) <= 3,
			"unlit materials changed during PBR migration");
}
} // namespace VulkanRenderer
