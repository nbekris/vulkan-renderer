#include "Application/Application.h"
#include "Rendering/ModernRendererTests.h"
#include "Unit/CameraTransformTests.h"
#include "Unit/FrustumTests.h"
#include "Rendering/Memory/AllocatedBuffer.h"
#include "Rendering/Frames/FrameRenderer.h"
#include "Rendering/Pipeline/GraphicsPipeline.h"
#include "Rendering/IDrawCommands.h"
#include "Rendering/Memory/MemoryAllocator.h"
#include "Rendering/Frames/FrameRing.h"
#include "Rendering/Frames/FrameResources.h"
#include "Rendering/Presentation/SwapChain.h"
#include "Rendering/Resources/Mesh.h"
#include "Rendering/Core/VulkanContext.h"
#include "Platform/Window.h"
#include "Rendering/Presentation/PresentationSession.h"
#include "Rendering/Resources/SceneResources.h"
#include "Rendering/Resources/Material.h"
#include "Rendering/Resources/Texture.h"
#include "Assets/MeshPrimitives.h"
#include "Scene/Scene.h"
#include <GLFW/glfw3.h>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

namespace VulkanRenderer {
namespace {
/** Substitutes scene drawing to test scheduling independently of the triangle. */
class TestDrawCommands : public IDrawCommands {
public:
	TestDrawCommands() = default;
	~TestDrawCommands() override = default;
	TestDrawCommands(const TestDrawCommands &) = delete;
	TestDrawCommands &operator=(const TestDrawCommands &) = delete;

	void Record(VkCommandBuffer commandBuffer, uint32_t frameIndex) const override {
		static_cast<void>(commandBuffer);
		static_cast<void>(frameIndex);
		if (_throwOnRecord) {
			throw std::runtime_error("injected drawing failure");
		}
	}

	void EnableFailure() { _throwOnRecord = true; }

private:
	bool _throwOnRecord = false;
};

void Require(bool condition, const char *message) {
	if (!condition) {
		throw std::runtime_error(message);
	}
}

void VerifyBufferOwnership(const MemoryAllocator &allocator) {
	{
		AllocatedBuffer buffer(allocator);
		const uint32_t DATA = 42;
		buffer.Initialize(sizeof(DATA), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
		buffer.Upload(&DATA, sizeof(DATA));
		bool rejected = false;
		try {
			buffer.Upload(&DATA, sizeof(DATA) + 1);
		} catch (const std::invalid_argument &) {
			rejected = true;
		}
		Require(rejected, "oversized uploads must be rejected before copying");
	}
	VmaTotalStatistics statistics{};
	vmaCalculateStatistics(allocator.GetHandle(), &statistics);
	Require(statistics.total.statistics.allocationCount == 0, "buffer destruction leaked a VMA allocation");
}

void VerifyPipelineFailure(const VulkanContext &context, const SwapChain &swapChain) {
	bool rejected = false;
	try {
		GraphicsPipeline pipeline(context);

		// The vertex module exists before loading the deliberately missing fragment module.
		pipeline.Initialize(swapChain.GetImageFormat(), swapChain.GetExtent(), "Shaders/vert.spv",
							"Shaders/missing-smoke-test.frag.spv");
	} catch (const std::runtime_error &exception) {
		rejected = std::string(exception.what()).find("missing-smoke-test.frag.spv") != std::string::npos;
	}
	Require(rejected, "missing fragment shader was not rejected");
}

void VerifyDrawingSubstitution(const VulkanContext &context, const MemoryAllocator &allocator,
							   const SwapChain &swapChain) {
	FrameRing frames(context, allocator);
	frames.Initialize(3);
	TestDrawCommands commands;
	bool rejected = false;
	try {
		FrameRenderer renderer(context, swapChain, frames, commands);
		renderer.Initialize();
		for (int frame = 0; frame < 4; ++frame) {
			renderer.DrawFrame();
		}
		commands.EnableFailure();
		// Unwind with earlier frames potentially in flight, after acquiring the next image.
		renderer.DrawFrame();
	} catch (const std::runtime_error &exception) {
		rejected = std::string(exception.what()) == "injected drawing failure";
	}
	Require(rejected, "draw command substitution did not propagate failure");
	// The same dependencies remain usable after the failed renderer is destroyed.
	FrameRing recoveryFrames(context, allocator);
	recoveryFrames.Initialize(2);
	TestDrawCommands recoveryCommands;
	FrameRenderer recoveryRenderer(context, swapChain, recoveryFrames, recoveryCommands);
	recoveryRenderer.Initialize();
	recoveryRenderer.DrawFrame();
}

template <typename Predicate> void WaitForWindowEvent(const Window &window, Predicate predicate) {
	const auto DEADLINE = std::chrono::steady_clock::now() + std::chrono::seconds(3);
	while (!predicate() && std::chrono::steady_clock::now() < DEADLINE) {
		glfwWaitEventsTimeout(0.01);
		window.PollEvents();
	}
	Require(predicate(), "timed out waiting for window event");
}

void VerifyPresentationResize(const VulkanContext &context, const MemoryAllocator &allocator, Window &window) {
	SceneResources resources(context, allocator);
	resources.Initialize();
	const auto mesh = resources.CreateMesh(MeshPrimitives::CreateCube());
	const std::array<uint8_t, 16> CHECKER{255, 255, 255, 255, 80, 80, 80, 255, 80, 80, 80, 255, 255, 255, 255, 255};
	MaterialData textured;
	textured.textureIndex = resources.AddTexture({2, 2}, CHECKER);
	const uint32_t MATERIAL_INDEX = resources.AddMaterial(textured);
	resources.Freeze();
	Scene scene;
	scene.AddObject(mesh, MATERIAL_INDEX);
	scene.GetCamera().SetPosition(glm::vec3(0, 0, 4));
	const auto VERTEX_ADDRESS = mesh->GetVertexAddress();
	const auto MATERIAL_BUFFER = resources.GetMaterial(MATERIAL_INDEX).GetBuffer();
	const auto TEXTURE_IMAGE = resources.GetTexture(textured.textureIndex).GetImage();
	const VkExtent2D SIZES[] = {{1024, 720}, {320, 240}, {1280, 800}, {800, 600}};
	for (const auto SIZE : SIZES) {
		{
			window.AcknowledgeFramebufferResize();
			PresentationSession oldPresentation(context, allocator, window, resources);
			oldPresentation.Initialize();
			oldPresentation.SetScene(scene);
			Require(oldPresentation.DrawFrame(), "failed to draw before resizing");
			glfwSetWindowSize(window.GetHandle(), static_cast<int>(SIZE.width), static_cast<int>(SIZE.height));
			WaitForWindowEvent(window, [&window] { return window.HasFramebufferResized(); });
			Require(!oldPresentation.DrawFrame(), "resize must retire the old presentation session");
		}
		Require(window.WaitUntilRenderable(), "resized window should be renderable");
		window.AcknowledgeFramebufferResize();
		PresentationSession newPresentation(context, allocator, window, resources);
		newPresentation.Initialize();
		newPresentation.SetScene(scene);
		const auto FRAMEBUFFER = window.GetFramebufferSize();
		const auto EXTENT = newPresentation.GetExtent();
		const auto DEPTH_EXTENT = newPresentation.GetDepthExtent();
		Require(DEPTH_EXTENT.width == EXTENT.width && DEPTH_EXTENT.height == EXTENT.height,
				"depth attachment extent was not updated on resize");
		Require(EXTENT.width == FRAMEBUFFER.width && EXTENT.height == FRAMEBUFFER.height,
				"swapchain extent did not follow framebuffer dimensions");
		for (int frame = 0; frame < 4; ++frame) {
			Require(newPresentation.DrawFrame(), "failed to draw after resizing");
			Require(mesh->GetVertexAddress() == VERTEX_ADDRESS
						&& resources.GetMaterial(MATERIAL_INDEX).GetBuffer() == MATERIAL_BUFFER
						&& resources.GetTexture(textured.textureIndex).GetImage() == TEXTURE_IMAGE,
					"resize replaced persistent mesh, material, or texture resources");
			Require(scene.GetCamera().GetPosition().z == 4, "resize reset camera state");
		}
	}

	{
		PresentationSession presentation(context, allocator, window, resources);
		presentation.Initialize();
		presentation.SetScene(scene);
		Require(presentation.DrawFrame(), "failed to draw before maximizing");
		glfwMaximizeWindow(window.GetHandle());
		WaitForWindowEvent(window, [&window] { return window.HasFramebufferResized(); });
		Require(!presentation.DrawFrame(), "maximizing must retire the old presentation session");
	}
	window.AcknowledgeFramebufferResize();
	{
		PresentationSession presentation(context, allocator, window, resources);
		presentation.Initialize();
		presentation.SetScene(scene);
		Require(presentation.DrawFrame(), "failed to draw before minimizing");
		glfwIconifyWindow(window.GetHandle());
		WaitForWindowEvent(window, [&window] { return !window.IsRenderable(); });
		Require(!presentation.DrawFrame(), "minimized windows must pause rendering");
		// Closing while minimized must not wait indefinitely for a nonzero framebuffer.
		glfwSetWindowShouldClose(window.GetHandle(), GLFW_TRUE);
		Require(!window.WaitUntilRenderable(), "closing a minimized window must exit the wait");
		glfwSetWindowShouldClose(window.GetHandle(), GLFW_FALSE);
	}
	glfwRestoreWindow(window.GetHandle());
	WaitForWindowEvent(window, [&window] { return window.IsRenderable(); });
	Require(window.WaitUntilRenderable(), "restored window should resume rendering");
	window.AcknowledgeFramebufferResize();
	PresentationSession restoredPresentation(context, allocator, window, resources);
	restoredPresentation.Initialize();
	restoredPresentation.SetScene(scene);
	Require(restoredPresentation.DrawFrame(), "failed to draw after restoring");
}
} // namespace

void RunRendererSmokeTests() {
	RunCameraTransformTests();
	RunFrustumTests();
	Application application;
	application.Run(8, 2);
	application.Run(8, 3);
	application.Run(8, 3, "Assets/Samples/SampleScene.glb", true);

	Window window;
	window.Initialize(800, 600, "Renderer smoke tests");
	VulkanContext context(window);
	context.Initialize();
	MemoryAllocator allocator(context);
	allocator.Initialize();
	{
		SwapChain swapChain(context, window);
		swapChain.Initialize();
		VerifyBufferOwnership(allocator);
		VerifyPipelineFailure(context, swapChain);
		VerifyDrawingSubstitution(context, allocator, swapChain);
		RunModernRendererTests(context, allocator, swapChain);
	}
	VerifyPresentationResize(context, allocator, window);
	VerifyBufferOwnership(allocator);
}
} // namespace VulkanRenderer

int main() {
	try {
		VulkanRenderer::RunRendererSmokeTests();
		std::cout << "Renderer smoke tests passed\n";
		return EXIT_SUCCESS;
	} catch (const std::exception &exception) {
		std::cerr << exception.what() << '\n';
		return EXIT_FAILURE;
	}
}
