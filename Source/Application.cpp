#include "Application.h"
#include "Window.h"
#include "VulkanContext.h"
#include "MemoryAllocator.h"
#include "PresentationSession.h"

namespace VulkanRenderer {

void Application::Run(int frameLimit) {
	// Declaration order is ownership order: dependencies outlive their consumers.
	Window window;
	window.Initialize(800, 600, "Vulkan Triangle");
	VulkanContext context(window);
	context.Initialize();
	MemoryAllocator allocator(context);
	allocator.Initialize();
	int frames = 0;
	while (!window.ShouldClose() && (frameLimit <= 0 || frames < frameLimit)) {
		if (!window.WaitUntilRenderable()) {
			break;
		}
		window.AcknowledgeFramebufferResize();
		PresentationSession presentation(context, allocator, window);
		presentation.Initialize();
		while (!window.ShouldClose() && (frameLimit <= 0 || frames < frameLimit)) {
			window.PollEvents();
			if (!presentation.DrawFrame()) {
				break;
			}
			++frames;
		}
		// Scope exit waits for the GPU before destroying this swapchain generation.
	}
	context.WaitIdle();
}

} // namespace VulkanRenderer
