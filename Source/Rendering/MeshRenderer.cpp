#include "Rendering/MeshRenderer.h"
#include "Rendering/Descriptors/GlobalDescriptors.h"
#include "Rendering/Presentation/SwapChain.h"
#include "Scene/Scene.h"
#include "Scene/SceneObject.h"
#include "Rendering/Resources/Mesh.h"
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <stdexcept>
#include "Math/Frustum.h"
#include <chrono>

namespace VulkanRenderer {
MeshRenderer::MeshRenderer(const VulkanContext &context, const GlobalDescriptors &descriptors)
	: _descriptors(descriptors), _pipeline(context) {
}

void MeshRenderer::Initialize(const SwapChain &swapChain, VkFormat depthFormat) {
	_pipeline.Initialize(swapChain.GetImageFormat(), swapChain.GetExtent(), "Shaders/vert.spv", "Shaders/frag.spv",
						 _descriptors.GetLayout(), depthFormat);
}

void MeshRenderer::ValidateScene(const Scene &scene) const {
	for (const auto &object : scene.GetObjects()) {
		if (!_descriptors.IsMaterialBound(object->GetMaterialIndex())) {
			throw std::out_of_range("scene object's material descriptor is not populated");
		}
	}
}

void MeshRenderer::SetScene(const Scene &scene) {
	ValidateScene(scene);
	_scene = &scene;
}

void MeshRenderer::Record(VkCommandBuffer commandBuffer, uint32_t frameIndex) const {
	_stats = {};
	const auto START = std::chrono::steady_clock::now();
	if (!_scene || _scene->GetObjects().empty()) {
		return;
	}
	// Scene edits are allowed between frames; reject unbound material indices before recording any draws.
	ValidateScene(*_scene);
	const Frustum FRUSTUM(_viewProjection);
	_pipeline.Bind(commandBuffer);
	_descriptors.Bind(commandBuffer, _pipeline.GetLayout());
	for (const auto &object : _scene->GetObjects()) {
		const auto MODEL = object->GetModelMatrix();
		if (_cullingEnabled && !FRUSTUM.IsVisible(object->GetMesh().GetBounds(), MODEL)) {
			++_stats.culled;
			continue;
		}
		++_stats.draws;
		_stats.triangles += object->GetMesh().GetIndexCount() / 3;
		DrawData draw{object->GetMesh().GetVertexAddress(), frameIndex, object->GetMaterialIndex()};
		std::copy_n(glm::value_ptr(MODEL), draw.model.size(), draw.model.begin());
		_pipeline.PushDrawData(commandBuffer, draw);
		object->GetMesh().Record(commandBuffer);
	}
	_stats.recordingMilliseconds
		= std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - START).count();
}
} // namespace VulkanRenderer
