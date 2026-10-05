#include "Application/Application.h"
#include "Platform/Window.h"
#include "Rendering/Core/VulkanContext.h"
#include "Rendering/Memory/MemoryAllocator.h"
#include "Rendering/Presentation/PresentationSession.h"
#include "Scene/Scene.h"
#include "Scene/CameraController.h"
#include "Assets/MeshPrimitives.h"
#include "Rendering/Resources/SceneResources.h"
#include "Rendering/Resources/Material.h"
#include "Assets/ModelLoader.h"
#include "Application/DemoScene.h"
#include "Diagnostics/Benchmark.h"
#include <iostream>
#include <vector>

namespace VulkanRenderer {

void Application::Run(int frameLimit, unsigned framesInFlight, const std::string &modelPath, bool benchmark,
					  bool culling, bool autoFrame) {
	// Declaration order is ownership order: dependencies outlive their consumers.
	Window window;
	window.Initialize(800, 600, "Vulkan 3D Mesh Renderer");
	VulkanContext context(window);
	context.Initialize();
	MemoryAllocator allocator(context);
	allocator.Initialize();
	SceneResources resources(context, allocator);
	resources.Initialize();
	Scene scene;
	if (modelPath.empty()) {
		DemoScene::Populate(resources, scene);
	}
	if (!modelPath.empty()) {
		const auto STATS = ModelLoader::Import(modelPath, resources, scene);
		std::cout << "Imported " << STATS.meshes << " meshes, " << STATS.materials << " materials, " << STATS.textures
				  << " textures, " << STATS.objects << " objects\n";
		if (autoFrame) {
			GltfLoader::FrameScene(scene, 800.0f / 600.0f);
		}
	}
	resources.Freeze();
	Benchmark measurements;
	CameraController controller;
	int frames = 0;
	while (!window.ShouldClose() && (frameLimit <= 0 || frames < frameLimit)) {
		if (!window.WaitUntilRenderable()) {
			break;
		}
		window.AcknowledgeFramebufferResize();
		PresentationSession presentation(context, allocator, window, resources, framesInFlight, benchmark);
		presentation.Initialize();
		presentation.SetScene(scene);
		presentation.EnableCulling(culling);
		while (!window.ShouldClose() && (frameLimit <= 0 || frames < frameLimit)) {
			window.PollEvents();
			controller.Update(window, scene.GetCamera());
			if (!presentation.DrawFrame()) {
				break;
			}
			++frames;
			if (benchmark) {
				measurements.Add(presentation.GetRenderStats(), presentation.GetGpuMilliseconds());
			}
		}
		// Scope exit waits for the GPU before destroying this swapchain generation.
	}
	context.WaitIdle();
	if (benchmark) {
		measurements.Print();
	}
}

} // namespace VulkanRenderer
