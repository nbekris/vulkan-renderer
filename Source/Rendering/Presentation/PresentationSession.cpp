#include "Rendering/Presentation/PresentationSession.h"
#include "Platform/Window.h"
#include "Scene/Scene.h"
#include "Rendering/Frames/DepthTarget.h"
#include "Rendering/Frames/FrameResources.h"
#include <glm/gtc/type_ptr.hpp>

namespace VulkanRenderer {

PresentationSession::PresentationSession(const VulkanContext &context, const MemoryAllocator &allocator,
										 const Window &window, const SceneResources &resources, uint32_t framesInFlight,
										 bool profile)
	: _context(context), _window(window), _FRAMES_IN_FLIGHT(framesInFlight), _PROFILE(profile),
	  _swapChain(context, window), _frames(context, allocator), _descriptors(context, resources, _frames),
	  _meshes(context, _descriptors), _renderer(context, _swapChain, _frames, _meshes) {
}

VkExtent2D PresentationSession::GetDepthExtent() const {
	return _frames.GetFrame(0).GetDepthTarget().GetExtent();
}

void PresentationSession::Initialize() {
	_swapChain.Initialize();
	const VkFormat DEPTH_FORMAT = DepthTarget::SelectFormat(_context);
	_frames.Initialize(_FRAMES_IN_FLIGHT, _swapChain.GetExtent(), DEPTH_FORMAT, _PROFILE);
	_descriptors.Initialize();
	_meshes.Initialize(_swapChain, DEPTH_FORMAT);
	_renderer.Initialize();
}

void PresentationSession::SetScene(const Scene &scene) {
	_scene = &scene;
	_meshes.SetScene(scene);
}

bool PresentationSession::DrawFrame() {
	if (_window.ShouldClose() || !_window.IsRenderable() || _window.HasFramebufferResized()) {
		return false;
	}
	const auto EXTENT = GetExtent();
	const auto UNIFORM
		= _scene ? _scene->GetFrameUniform(static_cast<float>(EXTENT.width) / EXTENT.height) : FrameUniform{};
	_meshes.SetViewProjection(glm::make_mat4(UNIFORM.transform.data()));
	return _renderer.DrawFrame(UNIFORM);
}

} // namespace VulkanRenderer
