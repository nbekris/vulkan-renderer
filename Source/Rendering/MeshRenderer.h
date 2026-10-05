#pragma once
#include "Rendering/IDrawCommands.h"
#include "Rendering/Pipeline/GraphicsPipeline.h"
#include "Math/Math.h"
#include "Rendering/ShadowPass.h"
#include <memory>

namespace VulkanRenderer {
class GlobalDescriptors;
class SwapChain;
class Scene;

struct RenderStats {
	uint32_t draws = 0;
	uint32_t culled = 0;
	uint64_t triangles = 0;
	double recordingMilliseconds = 0;
};

/** Records arbitrary scene meshes using one pipeline and one shared descriptor binding. */
class MeshRenderer : public IDrawCommands {
public:
	MeshRenderer(const VulkanContext &context, const GlobalDescriptors &descriptors);
	~MeshRenderer() override = default;
	MeshRenderer(const MeshRenderer &) = delete;
	MeshRenderer &operator=(const MeshRenderer &) = delete;
	void EnableShadows(const FrameRing &frames);
	void RecordBeforeRendering(VkCommandBuffer commandBuffer, uint32_t frameIndex) const override;
	void Record(VkCommandBuffer commandBuffer, uint32_t frameIndex) const override;

	const RenderStats &GetStats() const noexcept { return _stats; }

	void SetViewProjection(const glm::mat4 &matrix) noexcept { _viewProjection = matrix; }

	void EnableCulling(bool enabled) noexcept { _cullingEnabled = enabled; }

	void SetScene(const Scene &scene);
	void Initialize(const SwapChain &swapChain, VkFormat depthFormat = VK_FORMAT_UNDEFINED);

private:
	void ValidateScene(const Scene &scene) const;
	const GlobalDescriptors &_descriptors;
	GraphicsPipeline _pipeline;
	const VulkanContext &_context;
	std::unique_ptr<ShadowPass> _shadows;
	const Scene *_scene = nullptr;
	glm::mat4 _viewProjection{1};
	bool _cullingEnabled = false;
	mutable RenderStats _stats;
};
} // namespace VulkanRenderer
