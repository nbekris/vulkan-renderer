#include "Assets/GltfTests.h"
#include "Support/RenderProbe.h"
#include "Assets/GltfLoader.h"
#include "Scene/Scene.h"
#include "Rendering/Resources/SceneResources.h"
#include "Rendering/Resources/Mesh.h"
#include "Rendering/Frames/FrameRing.h"
#include "Rendering/Frames/FrameResources.h"
#include "Rendering/Descriptors/GlobalDescriptors.h"
#include "Rendering/MeshRenderer.h"
#include "Rendering/Memory/MemoryAllocator.h"
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace VulkanRenderer {
namespace {
void Require(bool condition, const char *message) {
	if (!condition) {
		throw std::runtime_error(message);
	}
}

uint32_t AllocationCount(const MemoryAllocator &allocator) {
	VmaTotalStatistics statistics{};
	vmaCalculateStatistics(allocator.GetHandle(), &statistics);
	return statistics.total.statistics.allocationCount;
}

void VerifyImportData() {
	for (const auto *path : {"Assets/Samples/SampleScene.gltf", "Assets/Samples/SampleScene.glb", "Tests/Assets/EmbeddedScene.gltf"}) {
		const auto MODEL = GltfLoader::Read(path);
		Require(MODEL.meshes.size() == 1 && MODEL.instances.size() == 2 && MODEL.textures.size() == 1
					&& MODEL.materials.size() == 1,
				"glTF resources were duplicated or omitted");
		Require(MODEL.meshes[0].vertices.size() == 24 && MODEL.meshes[0].indices.size() == 36,
				"glTF cube topology changed");
		Require(MODEL.textures[0].image.extent.width == 4 && MODEL.textures[0].image.pixels[1] == 230,
				"PNG image decoding or GLB buffer view failed");
		Require(MODEL.textures[0].sampling.magFilter == VK_FILTER_NEAREST
					&& MODEL.textures[0].sampling.addressU == VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
					&& MODEL.textures[0].sampling.addressV == VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
				"glTF sampler settings were lost");
		bool origin = false, translated = false;
		for (const auto &instance : MODEL.instances) {
			origin |= std::abs(instance.transform[3].x) < 0.001f;
			translated |= std::abs(instance.transform[3].x - 5.0f) < 0.001f;
		}
		Require(origin && translated && std::abs(MODEL.instances[1].transform[3].y) < 0.001f,
				"glTF node transforms were lost");
	}
	const auto MATERIAL_MODEL = GltfLoader::Read("Tests/Assets/MaterialScene.gltf");
	Require(MATERIAL_MODEL.materials[0].unlit == 1 && MATERIAL_MODEL.materials[0].alphaCutoff == 0.75f
				&& MATERIAL_MODEL.meshes[0].vertices[0].uv[0] == .25f
				&& MATERIAL_MODEL.meshes[0].vertices[0].uv[1] == .5f
				&& MATERIAL_MODEL.meshes[0].vertices[2].uv[0] == 2.25f
				&& MATERIAL_MODEL.meshes[0].vertices[2].uv[1] == 3.5f,
			"interleaved attributes, texture transforms, or glTF material flags failed");
	const auto SPARSE = GltfLoader::Read("Tests/Assets/SparseScene.gltf");
	Require(SPARSE.meshes[0].vertices[0].position[0] == -0.5f && SPARSE.meshes[0].vertices[0].color[1] == 1
				&& SPARSE.meshes[0].vertices[0].normal[2] == 1 && SPARSE.meshes[0].indices.size() == 3,
			"sparse attributes, normalized colors, or flat normals failed");
	for (const auto *path : {"Tests/Assets/UnsupportedScene.gltf", "Assets/DoesNotExist.gltf"}) {
		bool rejected = false;
		try {
			GltfLoader::Read(path);
		} catch (const std::runtime_error &) {
			rejected = true;
		}
		Require(rejected, "invalid glTF import was accepted");
	}
}
} // namespace

void RunGltfTests(const VulkanContext &context, const MemoryAllocator &allocator, const SwapChain &swapChain) {
	VerifyImportData();
	SceneResources resources(context, allocator);
	resources.Initialize();
	Scene scene;
	const auto BEFORE = resources.GetCheckpoint();
	const auto ALLOCATIONS = AllocationCount(allocator);
	bool rejected = false;
	try {
		GltfLoader::Import("Tests/Assets/InvalidMaterial.gltf", resources, scene);
	} catch (const std::invalid_argument &) {
		rejected = true;
	}
	const auto AFTER = resources.GetCheckpoint();
	Require(rejected && AFTER.meshes == BEFORE.meshes && AFTER.materials == BEFORE.materials
				&& AFTER.textures == BEFORE.textures && scene.GetObjects().empty()
				&& AllocationCount(allocator) == ALLOCATIONS,
			"failed import leaked or partially committed resources");
	const auto STATS = GltfLoader::Import("Assets/Samples/SampleScene.glb", resources, scene);
	Require(STATS.meshes == 1 && &scene.GetObject(0).GetMesh() == &scene.GetObject(1).GetMesh(),
			"glTF instances did not share GPU mesh allocations");
	GltfLoader::FrameScene(scene, 4.0f / 3.0f);
	Require(std::isfinite(scene.GetCamera().GetViewProjection(4.0f / 3.0f)[0][0]), "scene framing failed");
	// Use a fixed camera: one cube lies in the viewport and the translated cube is outside.
	scene.GetCamera().SetPosition(glm::vec3(0, 0, 3));
	Scene masked;
	GltfLoader::Import("Tests/Assets/MaterialScene.gltf", resources, masked);
	const uint32_t MASK_MATERIAL = masked.GetObject(1).GetMaterialIndex();
	MaterialData transparent;
	transparent.tint = {1, 1, 1, 0};
	transparent.alphaCutoff = .5f;
	transparent.unlit = 1;
	const auto TRANSPARENT = resources.AddMaterial(transparent);
	resources.Freeze();
	FrameRing frames(context, allocator);
	frames.Initialize(2);
	GlobalDescriptors descriptors(context, resources, frames);
	descriptors.Initialize();
	MeshRenderer renderer(context, descriptors);
	renderer.Initialize(swapChain);
	renderer.SetScene(scene);
	const auto UNIFORM
		= scene.GetFrameUniform(static_cast<float>(swapChain.GetExtent().width) / swapChain.GetExtent().height);
	renderer.SetViewProjection(glm::make_mat4(UNIFORM.transform.data()));
	RenderProbe probe(context, allocator, swapChain);
	probe.Initialize();
	auto &frame = frames.GetFrame(0);
	frame.Wait();
	frame.Prepare(UNIFORM);
	renderer.EnableCulling(true);
	probe.Render(renderer, frame, 0);
	const auto PIXEL = probe.GetCenterRgba();
	Require(PIXEL[1] > PIXEL[0] && PIXEL[1] > PIXEL[2] && PIXEL[1] > 40, "imported textured mesh failed GPU rendering");
	Require(renderer.GetStats().draws == 1 && renderer.GetStats().culled == 1 && renderer.GetStats().triangles == 12,
			"frustum culling counters are incorrect");
	renderer.EnableCulling(false);
	frame.Prepare(UNIFORM);
	probe.Render(renderer, frame, 0);
	Require(probe.GetCenterRgba() == PIXEL && renderer.GetStats().draws == 2 && renderer.GetStats().culled == 0,
			"culling changed visible pixels or disabling it failed");
	masked.GetCamera().SetPosition(glm::vec3(0, 0, 3));
	masked.GetLighting().SetAmbient(0);
	masked.GetLighting().SetIntensity(0);
	renderer.SetScene(masked);
	renderer.EnableCulling(false);
	frame.Prepare(
		masked.GetFrameUniform(static_cast<float>(swapChain.GetExtent().width) / swapChain.GetExtent().height));
	probe.Render(renderer, frame, 0);
	Require(probe.GetCenterRgba()[1] > 100, "KHR_materials_unlit did not bypass zero illumination");
	masked.GetObject(1).SetMaterialIndex(TRANSPARENT);
	frame.Prepare(UNIFORM);
	probe.Render(renderer, frame, 0);
	Require(probe.GetCenterRgba()[1] < 10, "alpha mask did not discard transparent geometry");
	masked.GetObject(1).SetMaterialIndex(MASK_MATERIAL);
}
} // namespace VulkanRenderer
