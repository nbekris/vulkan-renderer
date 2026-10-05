#include "Assets/AssimpTests.h"
#include "Assets/ModelLoader.h"
#include "Assets/AssimpLoader.h"
#include "Rendering/Resources/SceneResources.h"
#include "Rendering/Resources/Mesh.h"
#include "Rendering/Frames/FrameRing.h"
#include "Rendering/Descriptors/GlobalDescriptors.h"
#include "Rendering/MeshRenderer.h"
#include "Scene/Scene.h"
#include "Support/RenderProbe.h"
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <stdexcept>

namespace VulkanRenderer {
namespace {
void Require(bool condition, const char *message) {
	if (!condition) {
		throw std::runtime_error(message);
	}
}

template <typename Action> void Reject(Action action, const char *message) {
	bool rejected = false;
	try {
		action();
	} catch (const std::exception &) {
		rejected = true;
	}
	Require(rejected, message);
}

uint32_t Allocations(const MemoryAllocator &allocator) {
	VmaTotalStatistics statistics{};
	vmaCalculateStatistics(allocator.GetHandle(), &statistics);
	return statistics.total.statistics.allocationCount;
}
} // namespace

void RunAssimpTests(const VulkanContext &context, const MemoryAllocator &allocator, const SwapChain &swapChain) {
	const auto PLY = ModelLoader::Read("Tests/Assets/IndexedTetrahedron.ply");
	Require(PLY.meshes.size() == 1 && PLY.instances.size() == 1
		&& PLY.meshes[0].vertices.size() == 4
		&& PLY.meshes[0].indices == std::vector<uint32_t>{0, 2, 1, 0, 1, 3, 0, 3, 2, 1, 2, 3},
		"indexed PLY import changed surface connectivity or winding");
	for (const auto &vertex : PLY.meshes[0].vertices) {
		const glm::vec3 NORMAL(vertex.normal[0], vertex.normal[1], vertex.normal[2]);
		const glm::vec3 POSITION(vertex.position[0], vertex.position[1], vertex.position[2]);
		Require(std::abs(glm::length(NORMAL) - 1) < .001f && glm::dot(NORMAL, POSITION) > 0,
			"indexed PLY smooth normals are not outward unit vectors");
	}
	Reject([] { ModelLoader::Read("Tests/Assets/InvalidIndex.ply"); },
		"indexed PLY import accepted an out-of-range face index");
	const auto MODEL = ModelLoader::Read("Assets/Models/SamplePyramid");
	Require(MODEL.meshes.size() == 1 && MODEL.instances.size() == 1 && MODEL.textures.size() == 1,
			"Assimp folder import lost geometry, instances, or textures");
	Require(MODEL.meshes[0].indices.size() == 18, "Assimp failed to triangulate the pyramid base");
	for (const auto &vertex : MODEL.meshes[0].vertices) {
		const glm::vec3 NORMAL(vertex.normal[0], vertex.normal[1], vertex.normal[2]);
		Require(std::abs(glm::length(NORMAL) - 1) < .001f, "Assimp did not generate unit normals");
	}
	Require(MODEL.textures[0].image.extent.width == 4 && MODEL.textures[0].image.pixels[1] == 230,
			"Assimp external texture decoding failed");
	const auto GLTF = ModelLoader::Read("Assets/Samples/SampleScene.glb");
	Require(GLTF.instances.size() == 2 && GLTF.meshes.size() == 1, "model routing broke glTF import");
	// Exercise Assimp's matrix conversion and shared node mesh references using the existing GLB fixture.
	const auto ASSIMP_GLTF = AssimpLoader::Read("Assets/Samples/SampleScene.glb");
	bool translated = false;
	for (const auto &instance : ASSIMP_GLTF.instances) {
		translated |= std::abs(instance.transform[3].x - 5) < .001f;
	}
	Require(translated && ASSIMP_GLTF.instances.size() == 2 && ASSIMP_GLTF.meshes.size() == 1,
			"Assimp lost hierarchy transforms or duplicated a shared mesh");
	Require(ASSIMP_GLTF.textures.size() == 1, "Assimp embedded texture import failed");
	Reject([] { ModelLoader::Read("Assets/Models/Missing"); }, "missing model path was accepted");
	Reject([] { ModelLoader::ResolvePath("Assets/Samples"); }, "ambiguous model folder was accepted");
	Reject([] { ModelLoader::ResolvePath("Tests/Validation"); }, "empty model folder was accepted");
	Reject([] { ModelLoader::Read("Assets/Models/README.md"); }, "unsupported model file was accepted");
	const auto BASELINE = Allocations(allocator);
	{
		SceneResources resources(context, allocator);
		resources.Initialize();
		Scene scene;
		const auto BEFORE = resources.GetCheckpoint();
		const auto LIVE = Allocations(allocator);
		auto invalid = MODEL;
		invalid.materials[0].roughness = -1;
		Reject([&] { ModelLoader::Upload(invalid, resources, scene); }, "invalid Assimp material was accepted");
		const auto AFTER = resources.GetCheckpoint();
		Require(AFTER.meshes == BEFORE.meshes && AFTER.textures == BEFORE.textures
					&& AFTER.materials == BEFORE.materials && scene.GetObjects().empty()
					&& Allocations(allocator) == LIVE,
				"failed Assimp upload leaked or partially committed resources");
		const auto STATS = ModelLoader::Import("Assets/Models/SamplePyramid", resources, scene);
		Require(STATS.meshes == 1 && STATS.objects == 1 && STATS.textures == 1, "Assimp GPU upload failed");
		GltfLoader::FrameScene(scene, 4.0f / 3.0f);
		resources.Freeze();
		FrameRing frames(context, allocator);
		frames.Initialize(2);
		GlobalDescriptors descriptors(context, resources, frames);
		descriptors.Initialize();
		MeshRenderer renderer(context, descriptors);
		renderer.Initialize(swapChain);
		renderer.SetScene(scene);
		const auto UNIFORM = scene.GetFrameUniform(4.0f / 3.0f);
		renderer.SetViewProjection(glm::make_mat4(UNIFORM.transform.data()));
		RenderProbe probe(context, allocator, swapChain);
		probe.Initialize();
		auto &frame = frames.GetFrame(0);
		frame.Wait();
		frame.Prepare(UNIFORM);
		probe.Render(renderer, frame, 0);
		const auto PIXEL = probe.GetCenterRgba();
		Require(PIXEL[1] > 40 && PIXEL[1] > PIXEL[0] && PIXEL[1] > PIXEL[2],
				"Assimp textured pyramid did not render expected GPU pixels");
	}
	Require(Allocations(allocator) == BASELINE, "Assimp resource teardown leaked allocations");
}
} // namespace VulkanRenderer
