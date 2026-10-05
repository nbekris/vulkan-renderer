#pragma once
#include "Rendering/Presentation/SwapChain.h"
#include "Rendering/Frames/FrameRing.h"
#include "Rendering/Descriptors/GlobalDescriptors.h"
#include "Rendering/MeshRenderer.h"
#include "Rendering/Frames/FrameRenderer.h"

namespace VulkanRenderer {
class Scene;
class SceneResources;

/** Owns one generation of dynamic rendering resources and its shared descriptor table. */
class PresentationSession {
public:
	PresentationSession(const VulkanContext &context, const MemoryAllocator &allocator, const Window &window,
						const SceneResources &resources, uint32_t framesInFlight = 2, bool profile = false);
	virtual ~PresentationSession() = default;
	PresentationSession(const PresentationSession &) = delete;
	PresentationSession &operator=(const PresentationSession &) = delete;

	VkExtent2D GetExtent() const noexcept { return _swapChain.GetExtent(); }

	VkExtent2D GetDepthExtent() const;

	const RenderStats &GetRenderStats() const noexcept { return _meshes.GetStats(); }

	std::optional<double> GetGpuMilliseconds() const noexcept { return _renderer.GetLastGpuMilliseconds(); }

	void EnableCulling(bool enabled) noexcept { _meshes.EnableCulling(enabled); }

	void Initialize();
	void SetScene(const Scene &scene);
	bool DrawFrame();

private:
	const VulkanContext &_context;
	const Window &_window;
	const Scene *_scene = nullptr;
	const uint32_t _FRAMES_IN_FLIGHT;
	const bool _PROFILE;
	// Reverse destruction waits for GPU work before releasing descriptors, buffers and images.
	SwapChain _swapChain;
	FrameRing _frames;
	GlobalDescriptors _descriptors;
	MeshRenderer _meshes;
	FrameRenderer _renderer;
};
} // namespace VulkanRenderer
