#include "Rendering/ModernRendererTests.h"
#include "Support/RenderProbe.h"
#include "Rendering/Resources/SceneResources.h"
#include "Rendering/DepthBufferTests.h"
#include "Rendering/Memory/AllocatedBuffer.h"
#include "Rendering/Frames/FrameRing.h"
#include "Rendering/Frames/FrameResources.h"
#include "Rendering/Descriptors/GlobalDescriptors.h"
#include "Rendering/Memory/MemoryAllocator.h"
#include "Rendering/RenderData.h"
#include "Rendering/Presentation/SwapChain.h"
#include "Rendering/MeshRenderer.h"
#include "Rendering/Resources/Mesh.h"
#include "Assets/MeshPrimitives.h"
#include "Rendering/MeshTests.h"
#include "Assets/GltfTests.h"
#include "Assets/AssimpTests.h"
#include "Rendering/LightingTextureTests.h"
#include "Rendering/PbrTests.h"
#include "Rendering/ShadowTests.h"
#include "Rendering/Core/VulkanCheck.h"
#include "Rendering/Core/VulkanContext.h"
#include <memory>
#include "Scene/Scene.h"
#include <vector>

namespace VulkanRenderer {
namespace {
void Require(bool condition, const char *message) {
	if (!condition) {
		throw std::runtime_error(message);
	}
}

void VerifySuballocation(const MemoryAllocator &allocator) {
	std::vector<std::unique_ptr<AllocatedBuffer>> buffers;
	for (int i = 0; i < 128; ++i) {
		auto buffer = std::make_unique<AllocatedBuffer>(allocator);
		buffer->Initialize(sizeof(FrameUniform), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
		buffers.push_back(std::move(buffer));
	}
	VmaTotalStatistics statistics{};
	vmaCalculateStatistics(allocator.GetHandle(), &statistics);
	Require(statistics.total.statistics.allocationCount == 128, "expected 128 buffer suballocations");
	Require(statistics.total.statistics.blockCount < statistics.total.statistics.allocationCount,
			"buffers did not share VMA memory blocks");
}
} // namespace

void RunModernRendererTests(const VulkanContext &context, const MemoryAllocator &allocator,
							const SwapChain &swapChain) {
	VerifySuballocation(allocator);
	RunDepthBufferTests(context, allocator, swapChain);
	RunMeshTests(context, allocator, swapChain);
	RunLightingTextureTests(context, allocator, swapChain);
	RunPbrTests(context, allocator, swapChain);
	RunShadowTests(context, allocator, swapChain);
	RunGltfTests(context, allocator, swapChain);
	RunAssimpTests(context, allocator, swapChain);
	FrameRing frames(context, allocator);
	frames.Initialize(3);
	SceneResources resources(context, allocator);
	resources.Initialize();
	resources.Freeze();
	GlobalDescriptors descriptors(context, resources, frames);
	descriptors.Initialize();
	auto mesh = std::make_shared<Mesh>(context, allocator);
	const auto DATA = MeshPrimitives::CreateTriangle();
	mesh->Initialize(DATA.vertices, DATA.indices);
	Scene scene;
	scene.AddObject(mesh, 1);
	MeshRenderer renderer(context, descriptors);
	renderer.Initialize(swapChain);
	renderer.SetScene(scene);
	bool rejected = false;
	try {
		scene.GetObject(0).SetMaterialIndex(MATERIAL_CAPACITY - 1);
		renderer.SetScene(scene);
	} catch (const std::out_of_range &) {
		rejected = true;
	}
	Require(rejected, "unbound material indices must be rejected");
	scene.GetObject(0).SetMaterialIndex(1);
	renderer.SetScene(scene);
	RenderProbe probe(context, allocator, swapChain);
	probe.Initialize();
	for (uint32_t i = 0; i < frames.GetCount(); ++i) {
		auto &frame = frames.GetFrame(i);
		frame.Wait();
		FrameUniform uniform{};
		if (i == 1) {
			uniform.transform[12] = 2.0f;
		}
		frame.Prepare(uniform);
		probe.Render(renderer, frame, i);
		Require(probe.HasGreenCenter() == (i != 1),
				"GPU pixels disagree with vertex address, material index, or per-frame uniforms");
	}
	// A model transform must affect GPU output independently of camera/frame data.
	scene.AddObject(mesh, 1);
	scene.GetObject(0).GetTransform().SetPosition(glm::vec3(4, 0, 0));
	scene.GetObject(1).GetTransform().SetPosition(glm::vec3(-4, 0, 0));
	renderer.SetScene(scene);
	auto &frame = frames.GetFrame(0);
	frame.Wait();
	frame.Prepare(FrameUniform{});
	probe.Render(renderer, frame, 0);
	Require(!probe.HasGreenCenter(), "object transforms did not reach the vertex shader");
	scene.GetObject(0).GetTransform().SetPosition(glm::vec3(0));
	scene.GetObject(0).GetTransform().SetRotation(glm::vec3(0));
	frame.Prepare(FrameUniform{});
	probe.Render(renderer, frame, 0);
	Require(probe.HasGreenCenter(), "independent object transform did not restore visible geometry");
	// Isolate camera transforms from the view-dependent specular response.
	scene.GetLighting().SetIntensity(0);
	scene.GetLighting().SetAmbient(1);
	frame.Prepare(
		scene.GetFrameUniform(static_cast<float>(swapChain.GetExtent().width) / swapChain.GetExtent().height));
	probe.Render(renderer, frame, 0);
	Require(probe.HasGreenCenter(), "perspective camera did not render the object");
	scene.GetCamera().Move(glm::vec3(4, 0, 0));
	frame.Prepare(
		scene.GetFrameUniform(static_cast<float>(swapChain.GetExtent().width) / swapChain.GetExtent().height));
	probe.Render(renderer, frame, 0);
	Require(!probe.HasGreenCenter(), "camera movement did not affect shader output");
}
} // namespace VulkanRenderer
