#include "Rendering/ShadowPass.h"
#include "Rendering/Frames/FrameRing.h"
#include "Rendering/Frames/FrameResources.h"
#include "Rendering/Descriptors/GlobalDescriptors.h"
#include "Rendering/Resources/Mesh.h"
#include "Scene/Scene.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <limits>

namespace VulkanRenderer {
ShadowPass::ShadowPass(const VulkanContext &context, const GlobalDescriptors &descriptors, const FrameRing &frames)
	: _descriptors(descriptors), _frames(frames), _pipeline(context) {
}

void ShadowPass::Initialize() {
	_pipeline.Initialize(VK_FORMAT_UNDEFINED, {ShadowTarget::RESOLUTION, ShadowTarget::RESOLUTION},
						 "Shaders/shadow-vert.spv", "Shaders/shadow-frag.spv", _descriptors.GetLayout(),
						 _frames.GetFrame(0).GetShadowTarget().GetFormat());
}

glm::mat4 ShadowPass::GetLightTransform(const Scene &scene) {
	if (scene.GetObjects().empty()) {
		return glm::mat4(1);
	}
	const auto DIRECTION = scene.GetLighting().GetDirection();
	const glm::vec3 UP = std::abs(DIRECTION.y) > .95f ? glm::vec3(1, 0, 0) : glm::vec3(0, 1, 0);
	// Orientation only: fitting below determines translation and near/far depth in light space.
	const auto VIEW = glm::lookAt(DIRECTION, glm::vec3(0), UP);
	glm::vec3 minimum(std::numeric_limits<float>::max());
	glm::vec3 maximum(std::numeric_limits<float>::lowest());
	for (const auto &object : scene.GetObjects()) {
		const auto &BOUNDS = object->GetMesh().GetBounds();
		const auto MATRIX = VIEW * object->GetModelMatrix();
		for (uint32_t corner = 0; corner < 8; ++corner) {
			const glm::vec3 POINT = glm::vec3(MATRIX
											  * glm::vec4(corner & 1 ? BOUNDS.maximum.x : BOUNDS.minimum.x,
														  corner & 2 ? BOUNDS.maximum.y : BOUNDS.minimum.y,
														  corner & 4 ? BOUNDS.maximum.z : BOUNDS.minimum.z, 1));
			minimum = glm::min(minimum, POINT);
			maximum = glm::max(maximum, POINT);
		}
	}
	const auto PADDING = glm::max((maximum - minimum) * .05f, glm::vec3(.1f));
	minimum -= PADDING;
	maximum += PADDING;
	// Explicit right-handed orthographic projection with Vulkan zero-to-one depth.
	const auto SIZE = maximum - minimum;
	glm::mat4 projection(1);
	projection[0][0] = 2 / SIZE.x;
	projection[1][1] = 2 / SIZE.y;
	projection[2][2] = -1 / SIZE.z;
	projection[3][0] = -(maximum.x + minimum.x) / SIZE.x;
	projection[3][1] = -(maximum.y + minimum.y) / SIZE.y;
	projection[3][2] = maximum.z / SIZE.z;
	projection[1][1] *= -1;
	projection[3][1] *= -1;
	return projection * VIEW;
}

void ShadowPass::Record(VkCommandBuffer command, uint32_t frameIndex, const Scene &scene) const {
	const auto &TARGET = _frames.GetFrame(frameIndex).GetShadowTarget();
	TARGET.Transition(command, true);
	const auto ATTACHMENT = TARGET.GetAttachment();
	VkRenderingInfo rendering{};
	rendering.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	rendering.renderArea.extent = {ShadowTarget::RESOLUTION, ShadowTarget::RESOLUTION};
	rendering.layerCount = 1;
	rendering.pDepthAttachment = &ATTACHMENT;
	vkCmdBeginRendering(command, &rendering);
	_pipeline.Bind(command);
	_descriptors.Bind(command, _pipeline.GetLayout());
	// Include off-screen casters; camera frustum culling is inappropriate for shadow visibility.
	for (const auto &object : scene.GetObjects()) {
		DrawData draw{object->GetMesh().GetVertexAddress(), frameIndex, object->GetMaterialIndex()};
		const auto MODEL = object->GetModelMatrix();
		std::copy_n(glm::value_ptr(MODEL), draw.model.size(), draw.model.begin());
		_pipeline.PushDrawData(command, draw);
		object->GetMesh().Record(command);
	}
	vkCmdEndRendering(command);
	TARGET.Transition(command, false);
}
} // namespace VulkanRenderer
