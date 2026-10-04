#pragma once
#include "SwapChain.h"
#include "RenderTargets.h"
#include "TrianglePass.h"
#include "FrameRenderer.h"

namespace VulkanRenderer {
/** Owns one generation of size-dependent presentation resources. */
class PresentationSession {
public:
	PresentationSession(const VulkanContext &context, const MemoryAllocator &allocator, const Window &window);
	virtual ~PresentationSession() = default;
	PresentationSession(const PresentationSession &) = delete;
	PresentationSession &operator=(const PresentationSession &) = delete;

	VkExtent2D GetExtent() const noexcept { return _swapChain.GetExtent(); }

	void Initialize();
	bool DrawFrame();

private:
	const Window &_window;
	// Reverse destruction waits for frames, then releases their dependencies.
	SwapChain _swapChain;
	RenderTargets _targets;
	TrianglePass _triangle;
	FrameRenderer _renderer;
};
} // namespace VulkanRenderer
