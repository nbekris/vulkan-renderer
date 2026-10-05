#pragma once
#include "Rendering/Pipeline/GraphicsPipeline.h"
#include "Math/Math.h"

namespace VulkanRenderer {
class Scene;
class FrameRing;
class GlobalDescriptors;

/** Fits and records directional-light depth independently of camera visibility. */
class ShadowPass {
public:
	static glm::mat4 GetLightTransform(const Scene &scene);
	ShadowPass(const VulkanContext &context, const GlobalDescriptors &descriptors, const FrameRing &frames);
	virtual ~ShadowPass() = default;
	ShadowPass(const ShadowPass &) = delete;
	ShadowPass &operator=(const ShadowPass &) = delete;
	void Initialize();
	void Record(VkCommandBuffer command, uint32_t frameIndex, const Scene &scene) const;

private:
	const GlobalDescriptors &_descriptors;
	const FrameRing &_frames;
	GraphicsPipeline _pipeline;
};
} // namespace VulkanRenderer
