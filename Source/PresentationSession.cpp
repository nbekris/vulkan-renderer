#include "PresentationSession.h"
#include "Window.h"

namespace VulkanRenderer {

PresentationSession::PresentationSession(const VulkanContext &context, const MemoryAllocator &allocator,
										 const Window &window)
	: _window(window), _swapChain(context, window), _targets(context, _swapChain), _triangle(context, allocator),
	  _renderer(context, _swapChain, _targets, _triangle) {
}

void PresentationSession::Initialize() {
	_swapChain.Initialize();
	_targets.Initialize();
	_triangle.Initialize(_targets, _swapChain);
	_renderer.Initialize();
}

bool PresentationSession::DrawFrame() {
	// This check precedes acquisition, so no image semaphore has been signaled yet.
	if (_window.ShouldClose() || !_window.IsRenderable() || _window.HasFramebufferResized()) {
		return false;
	}
	return _renderer.DrawFrame();
}

} // namespace VulkanRenderer
