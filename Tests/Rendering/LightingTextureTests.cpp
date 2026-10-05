#include "Rendering/LightingTextureTests.h"
#include "Support/RenderProbe.h"
#include "Rendering/Frames/FrameRing.h"
#include "Rendering/Descriptors/GlobalDescriptors.h"
#include "Rendering/Resources/Material.h"
#include "Assets/MeshPrimitives.h"
#include "Rendering/MeshRenderer.h"
#include "Scene/Scene.h"
#include "Rendering/Resources/SceneResources.h"
#include "Rendering/Resources/Texture.h"
#include <cmath>

namespace VulkanRenderer {
namespace {
void Require(bool condition, const char *message) {
	if (!condition) {
		throw std::runtime_error(message);
	}
}

int OutputByte(float linear, VkFormat format) {
	if (format == VK_FORMAT_B8G8R8A8_SRGB || format == VK_FORMAT_R8G8B8A8_SRGB) {
		linear = linear <= 0.0031308f ? linear * 12.92f : 1.055f * std::pow(linear, 1.0f / 2.4f) - 0.055f;
	}
	return static_cast<int>(std::lround(linear * 255));
}

void VerifyMipGeneration() {
	const std::array<uint8_t, 16> CHECKER{0, 0, 0, 255, 255, 255, 255, 255, 255, 255, 255, 255, 0, 0, 0, 255};
	const auto SRGB = TexturePixels::BuildMipChain({2, 2}, CHECKER, TextureColorSpace::Srgb);
	const auto LINEAR = TexturePixels::BuildMipChain({2, 2}, CHECKER, TextureColorSpace::Linear);
	Require(SRGB.size() == 2 && SRGB.back().pixels[0] == 188 && SRGB.back().pixels[3] == 255,
			"sRGB mip generation did not average RGB in linear space");
	Require(LINEAR.back().pixels[0] == 128, "linear texture mip generation applied gamma correction");
	const std::array<uint8_t, 12> ODD{0, 0, 0, 255, 255, 255, 255, 255, 0, 0, 0, 255};
	const auto NPOT = TexturePixels::BuildMipChain({3, 1}, ODD, TextureColorSpace::Linear);
	Require(NPOT.back().pixels[0] == 85, "non-power-of-two mip generation discarded edge texels");
	bool rejected = false;
	try {
		TexturePixels::BuildMipChain({2, 2}, {}, TextureColorSpace::Srgb);
	} catch (const std::invalid_argument &) {
		rejected = true;
	}
	Require(rejected, "texture accepted an incorrect pixel byte count");
}

uint32_t AddTexturedMaterial(SceneResources &resources, std::span<const uint8_t> pixels,
							 TextureColorSpace colorSpace = TextureColorSpace::Srgb, VkExtent2D extent = {1, 1}) {
	MaterialData data;
	data.textureIndex = resources.AddTexture(extent, pixels, colorSpace);
	return resources.AddMaterial(data);
}
} // namespace

void RunLightingTextureTests(const VulkanContext &context, const MemoryAllocator &allocator,
							 const SwapChain &swapChain) {
	VerifyMipGeneration();
	SceneResources resources(context, allocator);
	resources.Initialize();
	const std::array<uint8_t, 4> RED{255, 0, 0, 255};
	const std::array<uint8_t, 4> GREEN{0, 255, 0, 255};
	const std::array<uint8_t, 4> GRAY{128, 128, 128, 255};
	const uint32_t RED_MATERIAL = AddTexturedMaterial(resources, RED);
	const uint32_t GREEN_MATERIAL = AddTexturedMaterial(resources, GREEN);
	const uint32_t SRGB_MATERIAL = AddTexturedMaterial(resources, GRAY);
	const uint32_t LINEAR_MATERIAL = AddTexturedMaterial(resources, GRAY, TextureColorSpace::Linear);
	std::vector<uint8_t> checker(4 * 4 * 4);
	for (size_t i = 0; i < 16; ++i) {
		const uint8_t VALUE = ((i / 4 + i % 4) % 2) ? 255 : 0;
		checker[i * 4] = checker[i * 4 + 1] = checker[i * 4 + 2] = VALUE;
		checker[i * 4 + 3] = 255;
	}
	const uint32_t CHECKER_MATERIAL = AddTexturedMaterial(resources, checker, TextureColorSpace::Srgb, {4, 4});
	auto triangle = MeshPrimitives::CreateTriangle();
	for (auto &vertex : triangle.vertices) {
		vertex.color = {1, 1, 1, 1};
	}
	const auto mesh = resources.CreateMesh(triangle);
	for (auto &vertex : triangle.vertices) {
		vertex.uv[0] *= 8192;
		vertex.uv[1] *= 8192;
	}
	const auto minified = resources.CreateMesh(triangle);
	bool rejected = false;
	try {
		MaterialData invalid;
		invalid.textureIndex = TEXTURE_CAPACITY - 1;
		resources.AddMaterial(invalid);
	} catch (const std::out_of_range &) {
		rejected = true;
	}
	Require(rejected, "material accepted an unpopulated texture index");
	Require(resources.GetTexture(5).GetMipLevels() == 3, "texture did not upload its full mip chain");
	resources.Freeze();
	rejected = false;
	try {
		resources.AddTexture({1, 1}, RED);
	} catch (const std::logic_error &) {
		rejected = true;
	}
	Require(rejected, "frozen resources accepted a texture-table mutation");
	rejected = false;
	try {
		resources.AddMaterial(MaterialData{});
	} catch (const std::logic_error &) {
		rejected = true;
	}
	Require(rejected, "frozen resources accepted a material-table mutation");

	FrameRing frames(context, allocator);
	frames.Initialize(3);
	GlobalDescriptors descriptors(context, resources, frames);
	descriptors.Initialize();
	Scene scene;
	scene.AddObject(mesh, RED_MATERIAL);
	scene.GetLighting().SetColor(glm::vec3(1));
	scene.GetLighting().SetDirection(glm::vec3(0, 0, 1));
	MeshRenderer renderer(context, descriptors);
	renderer.Initialize(swapChain);
	renderer.SetScene(scene);
	RenderProbe probe(context, allocator, swapChain);
	probe.Initialize();
	auto &frame = frames.GetFrame(0);
	const auto RENDER = [&](const FrameUniform &uniform) {
		frame.Wait();
		frame.Prepare(uniform);
		probe.Render(renderer, frame, 0);
		return probe.GetCenterRgba();
	};
	auto pixel = RENDER(FrameUniform{});
	Require(pixel[0] > 245 && pixel[1] < 3 && pixel[2] < 3, "material did not select the red texture");
	scene.GetObject(0).SetMaterialIndex(GREEN_MATERIAL);
	Require(RENDER(FrameUniform{})[1] > 245 && probe.HasGreenCenter(), "material did not select the green texture");
	scene.GetObject(0).SetMaterialIndex(SRGB_MATERIAL);
	const auto SRGB_PIXEL = RENDER(FrameUniform{});
	scene.GetObject(0).SetMaterialIndex(LINEAR_MATERIAL);
	const auto LINEAR_PIXEL = RENDER(FrameUniform{});
	const float DECODED = std::pow((128.0f / 255 + 0.055f) / 1.055f, 2.4f);
	Require(std::abs(static_cast<int>(SRGB_PIXEL[0]) - OutputByte(DECODED, swapChain.GetImageFormat())) <= 3
				&& std::abs(static_cast<int>(LINEAR_PIXEL[0]) - OutputByte(128.0f / 255, swapChain.GetImageFormat()))
					   <= 3,
			"sampled textures did not distinguish sRGB color from linear data");

	scene.GetObject(0).SetMaterialIndex(0);
	scene.GetLighting().SetAmbient(0);
	scene.GetLighting().SetIntensity(1);
	auto uniform = scene.GetFrameUniform(1);
	uniform.transform = FrameUniform{}.transform;
	Require(std::abs(static_cast<int>(RENDER(uniform)[0]) - OutputByte(0.356507f, swapChain.GetImageFormat())) <= 3,
			"aligned directional light did not illuminate the surface");
	scene.GetLighting().SetDirection(glm::vec3(0, 0, -1));
	uniform = scene.GetFrameUniform(1);
	uniform.transform = FrameUniform{}.transform;
	Require(RENDER(uniform)[0] < 3, "back-facing diffuse illumination was not clamped to zero");
	scene.GetLighting().SetAmbient(0.25f);
	uniform = scene.GetFrameUniform(1);
	uniform.transform = FrameUniform{}.transform;
	pixel = RENDER(uniform);
	Require(std::abs(static_cast<int>(pixel[0]) - OutputByte(0.25f, swapChain.GetImageFormat())) <= 3,
			"ambient intensity was not applied in linear space");
	// Every frame slot receives independent fragment-stage lighting uniforms.
	for (uint32_t i = 0; i < frames.GetCount(); ++i) {
		auto &slot = frames.GetFrame(i);
		slot.Wait();
		uniform = FrameUniform{};
		uniform.lightDirection[3] = static_cast<float>(i) * 0.5f;
		uniform.lightColor = {1, 0, 0, 0};
		slot.Prepare(uniform);
		probe.Render(renderer, slot, i);
		pixel = probe.GetCenterRgba();
		Require(std::abs(static_cast<int>(pixel[0])
						 - OutputByte(0.356507f * static_cast<float>(i) * 0.5f, swapChain.GetImageFormat()))
						<= 3
					&& pixel[1] < 3 && pixel[2] < 3,
				"per-frame light intensity/color did not reach the fragment shader");
	}
	Scene tiny;
	tiny.AddObject(minified, CHECKER_MATERIAL);
	renderer.SetScene(tiny);
	pixel = RENDER(FrameUniform{});
	Require(std::abs(static_cast<int>(pixel[0]) - OutputByte(0.5f, swapChain.GetImageFormat())) <= 4,
			"minification did not sample the gamma-correct final mip level");
	rejected = false;
	try {
		scene.GetLighting().SetDirection(glm::vec3(0));
	} catch (const std::invalid_argument &) {
		rejected = true;
	}
	Require(rejected, "light accepted a zero direction");
}
} // namespace VulkanRenderer
