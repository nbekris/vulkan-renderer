#include "Rendering/DepthBufferTests.h"
#include "Support/RenderProbe.h"
#include "Rendering/Resources/SceneResources.h"
#include "Rendering/Frames/DepthTarget.h"
#include "Rendering/Frames/FrameRing.h"
#include "Rendering/Descriptors/GlobalDescriptors.h"
#include "Rendering/Pipeline/GraphicsPipeline.h"
#include "Rendering/Resources/Mesh.h"
#include "Assets/MeshPrimitives.h"
#include <array>

namespace VulkanRenderer {
namespace {
void Require(bool condition, const char *message) {
	if (!condition) {
		throw std::runtime_error(message);
	}
}

/** Draws overlapping triangles with distinct materials in a configurable order. */
class DepthDraw : public IDrawCommands {
public:
	DepthDraw(const VulkanContext &context, const MemoryAllocator &allocator, const GlobalDescriptors &descriptors)
		: _descriptors(descriptors), _pipeline(context), _mesh(context, allocator) {}

	~DepthDraw() override = default;
	DepthDraw(const DepthDraw &) = delete;
	DepthDraw &operator=(const DepthDraw &) = delete;

	void Initialize(const SwapChain &swapChain, VkFormat depthFormat) {
		const auto DATA = MeshPrimitives::CreateTriangle();
		_mesh.Initialize(DATA.vertices, DATA.indices);
		_pipeline.Initialize(swapChain.GetImageFormat(), swapChain.GetExtent(), "Shaders/vert.spv", "Shaders/frag.spv",
							 _descriptors.GetLayout(), depthFormat);
	}

	void Configure(bool greenFirst, float greenDepth, float whiteDepth) {
		_greenFirst = greenFirst;
		_greenDepth = greenDepth;
		_whiteDepth = whiteDepth;
	}

	void Record(VkCommandBuffer commandBuffer, uint32_t frameIndex) const override {
		_pipeline.Bind(commandBuffer);
		_descriptors.Bind(commandBuffer, _pipeline.GetLayout());
		for (uint32_t i = 0; i < 2; ++i) {
			const bool GREEN = (i == 0) == _greenFirst;
			DrawData draw{_mesh.GetVertexAddress(), frameIndex, GREEN ? 1u : 0u};
			draw.model[14] = GREEN ? _greenDepth : _whiteDepth;
			_pipeline.PushDrawData(commandBuffer, draw);
			_mesh.Record(commandBuffer);
		}
	}

private:
	const GlobalDescriptors &_descriptors;
	GraphicsPipeline _pipeline;
	Mesh _mesh;
	bool _greenFirst = true;
	float _greenDepth = 0.2f;
	float _whiteDepth = 0.8f;
};

void VerifyDepthFormat(const VulkanContext &context, const MemoryAllocator &allocator, const SwapChain &swapChain,
					   VkFormat format) {
	FrameRing frames(context, allocator);
	frames.Initialize(3, swapChain.GetExtent(), format);
	SceneResources resources(context, allocator);
	resources.Initialize();
	resources.Freeze();
	GlobalDescriptors descriptors(context, resources, frames);
	descriptors.Initialize();
	DepthDraw draw(context, allocator, descriptors);
	draw.Initialize(swapChain, format);
	RenderProbe probe(context, allocator, swapChain);
	probe.Initialize();
	std::array<VkImage, 3> images{};
	for (uint32_t i = 0; i < frames.GetCount(); ++i) {
		auto &frame = frames.GetFrame(i);
		images[i] = frame.GetDepthTarget().GetImage();
		Require(images[i] != VK_NULL_HANDLE, "frame slot has no depth image");
		for (bool greenFirst : {true, false}) {
			frame.Wait();
			frame.Prepare(FrameUniform{});
			draw.Configure(greenFirst, 0.2f, 0.8f);
			probe.Render(draw, frame, i);
			Require(probe.HasGreenCenter(), "near geometry was overwritten by a farther draw");
		}
		// Swap depths on the same attachment: it must clear and accept the newly nearer material.
		frame.Prepare(FrameUniform{});
		draw.Configure(true, 0.8f, 0.2f);
		probe.Render(draw, frame, i);
		Require(probe.HasMulticolorCenter(), "depth clear or depth writes failed on attachment reuse");
		frame.Prepare(FrameUniform{});
		draw.Configure(false, -0.2f, 0.8f);
		probe.Render(draw, frame, i);
		Require(probe.HasMulticolorCenter(), "geometry in front of the near plane was not clipped");
	}
	Require(images[0] != images[1] && images[1] != images[2] && images[0] != images[2],
			"frames in flight share a depth image");
}
} // namespace

void RunDepthBufferTests(const VulkanContext &context, const MemoryAllocator &allocator, const SwapChain &swapChain) {
	const auto FORMAT = DepthTarget::SelectFormat(context);
	VerifyDepthFormat(context, allocator, swapChain, FORMAT);
	// Exercise the combined depth/stencil transition without enabling separate depth/stencil layouts.
	VkFormatProperties properties{};
	vkGetPhysicalDeviceFormatProperties(context.GetPhysicalDevice(), VK_FORMAT_D24_UNORM_S8_UINT, &properties);
	if (FORMAT != VK_FORMAT_D24_UNORM_S8_UINT
		&& (properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)) {
		VerifyDepthFormat(context, allocator, swapChain, VK_FORMAT_D24_UNORM_S8_UINT);
	}
}
} // namespace VulkanRenderer
