#include "Rendering/Presentation/PresentationSession.h"
#include "Platform/Window.h"
#include "Scene/Scene.h"
#include "Rendering/Frames/DepthTarget.h"
#include "Rendering/Frames/FrameResources.h"
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>

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
	_frames.Initialize(_FRAMES_IN_FLIGHT, _swapChain.GetExtent(), DEPTH_FORMAT, _PROFILE, true);
	_descriptors.Initialize();
	_meshes.Initialize(_swapChain, DEPTH_FORMAT);
	_meshes.EnableShadows(_frames);
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
	auto uniform = _scene ? _scene->GetFrameUniform(static_cast<float>(EXTENT.width) / EXTENT.height) : FrameUniform{};
	_meshes.SetViewProjection(glm::make_mat4(uniform.transform.data()));
	if (_scene) {
		const auto LIGHT_TRANSFORM = ShadowPass::GetLightTransform(*_scene);
		std::copy_n(glm::value_ptr(LIGHT_TRANSFORM), uniform.lightTransform.size(), uniform.lightTransform.begin());
		uniform.shadowParameters[0] = 1;
	}
	return _renderer.DrawFrame(uniform);
}

} // namespace VulkanRenderer
