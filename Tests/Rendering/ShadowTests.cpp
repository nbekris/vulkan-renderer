#include "Rendering/ShadowTests.h"
#include "Support/RenderProbe.h"
#include "Rendering/Resources/SceneResources.h"
#include "Rendering/Resources/Material.h"
#include "Rendering/Frames/FrameRing.h"
#include "Rendering/Frames/DepthTarget.h"
#include "Rendering/Descriptors/GlobalDescriptors.h"
#include "Rendering/MeshRenderer.h"
#include "Assets/MeshPrimitives.h"
#include "Scene/Scene.h"
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>

namespace VulkanRenderer {
void RunShadowTests(const VulkanContext &context, const MemoryAllocator &allocator, const SwapChain &swapChain) {
	SceneResources resources(context, allocator);
	resources.Initialize();
	MaterialData material;
	material.roughness = 1;
	const auto OPAQUE = resources.AddMaterial(material);
	material.alphaCutoff = .5f;
	material.tint[3] = 0;
	const auto TRANSPARENT = resources.AddMaterial(material);
	auto geometry = MeshPrimitives::CreateTriangle();
	for (auto &vertex : geometry.vertices) {
		vertex.color = {1, 1, 1, 1};
	}
	const auto MESH = resources.CreateMesh(geometry);
	resources.Freeze();
	Scene scene;
	scene.GetLighting().SetDirection(glm::vec3(0, 0, 1));
	scene.AddObject(MESH, OPAQUE).GetTransform().SetPosition(glm::vec3(0, 0, .5f));
	// The caster is outside camera clip depth but between the receiver and light.
	scene.AddObject(MESH, OPAQUE).GetTransform().SetPosition(glm::vec3(0, 0, 2));
	FrameRing frames(context, allocator);
	const auto DEPTH = DepthTarget::SelectFormat(context);
	frames.Initialize(3, swapChain.GetExtent(), DEPTH, false, true);
	GlobalDescriptors descriptors(context, resources, frames);
	descriptors.Initialize();
	MeshRenderer renderer(context, descriptors);
	renderer.Initialize(swapChain, DEPTH);
	renderer.EnableShadows(frames);
	renderer.EnableCulling(true);
	renderer.SetScene(scene);
	RenderProbe probe(context, allocator, swapChain);
	probe.Initialize();
	FrameUniform uniform;
	uniform.lightDirection = {0, 0, 1, 1};
	uniform.lightColor = {1, 1, 1, .1f};
	uniform.shadowParameters[0] = 1;
	const auto RENDER = [&](uint32_t index) {
		const auto LIGHT = ShadowPass::GetLightTransform(scene);
		std::copy_n(glm::value_ptr(LIGHT), 16, uniform.lightTransform.begin());
		auto &frame = frames.GetFrame(index);
		frame.Wait();
		frame.Prepare(uniform);
		probe.Render(renderer, frame, index);
		return probe.GetCenterRgba()[0];
	};
	for (uint32_t index = 0; index < 3; ++index) {
		const auto SHADOWED = RENDER(index);
		uniform.shadowParameters[0] = 0;
		const auto LIT = RENDER(index);
		if (LIT < SHADOWED + 35 || SHADOWED < 15) {
			throw std::runtime_error("off-camera caster failed to shadow direct lighting or ambient was shadowed");
		}
		uniform.shadowParameters[0] = 1;
		// A deliberately wide kernel straddles the caster edge and must yield fractional visibility.
		uniform.shadowParameters[3] = .3f;
		const auto FILTERED = RENDER(index);
		if (FILTERED <= SHADOWED + 5 || FILTERED >= LIT - 5) {
			throw std::runtime_error("PCF did not average lit and shadowed depth comparisons");
		}
		uniform.shadowParameters[3] = 1.0f / ShadowTarget::RESOLUTION;
		scene.GetObject(1).SetMaterialIndex(TRANSPARENT);
		if (RENDER(index) < LIT - 3) {
			throw std::runtime_error("alpha cutout cast an opaque shadow");
		}
		scene.GetObject(1).SetMaterialIndex(OPAQUE);
	}
	scene.GetObject(1).GetTransform().SetPosition(glm::vec3(5, 0, 2));
	const auto MOVED = RENDER(0);
	uniform.shadowParameters[0] = 0;
	if (RENDER(0) > MOVED + 3) {
		throw std::runtime_error("moving caster left a stale shadow");
	}
}
} // namespace VulkanRenderer
