#include "../Source/Application.h"
#include "../Source/AllocatedBuffer.h"
#include "../Source/FrameRenderer.h"
#include "../Source/GraphicsPipeline.h"
#include "../Source/IDrawCommands.h"
#include "../Source/MemoryAllocator.h"
#include "../Source/RenderTargets.h"
#include "../Source/SwapChain.h"
#include "../Source/TriangleMesh.h"
#include "../Source/VulkanContext.h"
#include "../Source/Window.h"
#include "../Source/PresentationSession.h"
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

	void Record(VkCommandBuffer commandBuffer) const override {
		static_cast<void>(commandBuffer);
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

void VerifyPipelineFailure(const VulkanContext &context, const RenderTargets &targets, const SwapChain &swapChain) {
	bool rejected = false;
	try {
		GraphicsPipeline pipeline(context);
		const auto BINDING = TriangleMesh::GetBinding();
		const auto ATTRIBUTES = TriangleMesh::GetAttributes();
		// The vertex module exists before loading the deliberately missing fragment module.
		pipeline.Initialize(targets.GetRenderPass(), swapChain.GetExtent(), BINDING, ATTRIBUTES, "Shaders/vert.spv",
							"Shaders/missing-smoke-test.frag.spv");
	} catch (const std::runtime_error &exception) {
		rejected = std::string(exception.what()).find("missing-smoke-test.frag.spv") != std::string::npos;
	}
	Require(rejected, "missing fragment shader was not rejected");
}

void VerifyDrawingSubstitution(const VulkanContext &context, const RenderTargets &targets, const SwapChain &swapChain) {
	TestDrawCommands commands;
	bool rejected = false;
	try {
		FrameRenderer renderer(context, swapChain, targets, commands);
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
	TestDrawCommands recoveryCommands;
	FrameRenderer recoveryRenderer(context, swapChain, targets, recoveryCommands);
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
	const VkExtent2D SIZES[] = {{1024, 720}, {320, 240}, {1280, 800}, {800, 600}};
	for (const auto SIZE : SIZES) {
		{
			window.AcknowledgeFramebufferResize();
			PresentationSession oldPresentation(context, allocator, window);
			oldPresentation.Initialize();
			Require(oldPresentation.DrawFrame(), "failed to draw before resizing");
			glfwSetWindowSize(window.GetHandle(), static_cast<int>(SIZE.width), static_cast<int>(SIZE.height));
			WaitForWindowEvent(window, [&window] { return window.HasFramebufferResized(); });
			Require(!oldPresentation.DrawFrame(), "resize must retire the old presentation session");
		}
		Require(window.WaitUntilRenderable(), "resized window should be renderable");
		window.AcknowledgeFramebufferResize();
		PresentationSession newPresentation(context, allocator, window);
		newPresentation.Initialize();
		const auto FRAMEBUFFER = window.GetFramebufferSize();
		const auto EXTENT = newPresentation.GetExtent();
		Require(EXTENT.width == FRAMEBUFFER.width && EXTENT.height == FRAMEBUFFER.height,
				"swapchain extent did not follow framebuffer dimensions");
		for (int frame = 0; frame < 4; ++frame) {
			Require(newPresentation.DrawFrame(), "failed to draw after resizing");
		}
	}

	{
		PresentationSession presentation(context, allocator, window);
		presentation.Initialize();
		Require(presentation.DrawFrame(), "failed to draw before maximizing");
		glfwMaximizeWindow(window.GetHandle());
		WaitForWindowEvent(window, [&window] { return window.HasFramebufferResized(); });
		Require(!presentation.DrawFrame(), "maximizing must retire the old presentation session");
	}
	window.AcknowledgeFramebufferResize();
	{
		PresentationSession presentation(context, allocator, window);
		presentation.Initialize();
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
	PresentationSession restoredPresentation(context, allocator, window);
	restoredPresentation.Initialize();
	Require(restoredPresentation.DrawFrame(), "failed to draw after restoring");
}
} // namespace

void RunRendererSmokeTests() {
	Application application;
	application.Run(2);
	application.Run(2);

	Window window;
	window.Initialize(800, 600, "Renderer smoke tests");
	VulkanContext context(window);
	context.Initialize();
	MemoryAllocator allocator(context);
	allocator.Initialize();
	{
		SwapChain swapChain(context, window);
		swapChain.Initialize();
		RenderTargets targets(context, swapChain);
		targets.Initialize();
		VerifyBufferOwnership(allocator);
		VerifyPipelineFailure(context, targets, swapChain);
		VerifyDrawingSubstitution(context, targets, swapChain);
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
