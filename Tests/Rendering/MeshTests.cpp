#include "Rendering/MeshTests.h"
#include "Support/RenderProbe.h"
#include "Rendering/Resources/SceneResources.h"
#include "Rendering/Resources/Mesh.h"
#include "Assets/MeshPrimitives.h"
#include "Rendering/MeshRenderer.h"
#include "Scene/Scene.h"
#include "Rendering/Frames/FrameRing.h"
#include "Rendering/Descriptors/GlobalDescriptors.h"
#include <cmath>

namespace VulkanRenderer {
namespace {
void Require(bool condition, const char *message) {
	if (!condition) {
		throw std::runtime_error(message);
	}
}

void VerifyCubeGeometry() {
	const auto DATA = MeshPrimitives::CreateCube();
	Require(DATA.vertices.size() == 24 && DATA.indices.size() == 36, "cube must have six distinct indexed faces");
	for (size_t i = 0; i < DATA.indices.size(); i += 3) {
		const auto &A = DATA.vertices.at(DATA.indices[i]);
		const auto &B = DATA.vertices.at(DATA.indices[i + 1]);
		const auto &C = DATA.vertices.at(DATA.indices[i + 2]);
		const glm::vec3 AB(B.position[0] - A.position[0], B.position[1] - A.position[1], B.position[2] - A.position[2]);
		const glm::vec3 AC(C.position[0] - A.position[0], C.position[1] - A.position[1], C.position[2] - A.position[2]);
		const glm::vec3 NORMAL(A.normal[0], A.normal[1], A.normal[2]);
		Require(glm::dot(glm::normalize(glm::cross(AB, AC)), NORMAL) > 0.999f,
				"cube normals do not match outward triangle winding");
	}
	for (const auto &vertex : DATA.vertices) {
		Require(vertex.uv[0] >= 0 && vertex.uv[0] <= 1 && vertex.uv[1] >= 0 && vertex.uv[1] <= 1,
				"cube UVs are outside the face texture domain");
	}
}

/** Uses a fragment probe to verify actual device-address normal/UV reads and inverse-transpose normals. */
class AttributeDraw : public IDrawCommands {
public:
	AttributeDraw(const VulkanContext &context, const GlobalDescriptors &descriptors, const Mesh &mesh)
		: _descriptors(descriptors), _pipeline(context), _mesh(mesh) {}

	~AttributeDraw() override = default;
	AttributeDraw(const AttributeDraw &) = delete;
	AttributeDraw &operator=(const AttributeDraw &) = delete;

	void Initialize(const SwapChain &swapChain) {
		_pipeline.Initialize(swapChain.GetImageFormat(), swapChain.GetExtent(), "Shaders/vert.spv",
							 "Shaders/mesh-attributes.spv", _descriptors.GetLayout());
	}

	void Record(VkCommandBuffer commandBuffer, uint32_t frameIndex) const override {
		_pipeline.Bind(commandBuffer);
		_descriptors.Bind(commandBuffer, _pipeline.GetLayout());
		DrawData draw{_mesh.GetVertexAddress(), frameIndex, 0};
		draw.model[0] = 2;
		_pipeline.PushDrawData(commandBuffer, draw);
		_mesh.Record(commandBuffer);
	}

private:
	const GlobalDescriptors &_descriptors;
	GraphicsPipeline _pipeline;
	const Mesh &_mesh;
};
} // namespace

void RunMeshTests(const VulkanContext &context, const MemoryAllocator &allocator, const SwapChain &swapChain) {
	VerifyCubeGeometry();
	Mesh invalid(context, allocator);
	auto triangleData = MeshPrimitives::CreateTriangle();
	bool rejected = false;
	try {
		invalid.Initialize(triangleData.vertices, std::array<uint32_t, 3>{0, 1, 3});
	} catch (const std::out_of_range &) {
		rejected = true;
	}
	Require(rejected && !invalid.IsInitialized(), "mesh accepted out-of-range indices");
	rejected = false;
	try {
		invalid.Initialize({}, {});
	} catch (const std::invalid_argument &) {
		rejected = true;
	}
	Require(rejected, "mesh accepted empty data");
	invalid.Initialize(triangleData.vertices, triangleData.indices);
	rejected = false;
	try {
		invalid.Initialize(triangleData.vertices, triangleData.indices);
	} catch (const std::logic_error &) {
		rejected = true;
	}
	Require(rejected, "mesh accepted a second initialization");
	rejected = false;
	try {
		SceneObject missing(nullptr, 0);
	} catch (const std::invalid_argument &) {
		rejected = true;
	}
	Require(rejected, "scene object accepted a missing mesh");

	FrameRing frames(context, allocator);
	frames.Initialize(2);
	SceneResources resources(context, allocator);
	resources.Initialize();
	resources.Freeze();
	GlobalDescriptors descriptors(context, resources, frames);
	descriptors.Initialize();
	RenderProbe probe(context, allocator, swapChain);
	probe.Initialize();
	auto &frame = frames.GetFrame(0);
	// Use indices beyond UINT16_MAX to catch index-type truncation and arbitrary vertex-count regressions.
	MeshData large;
	large.vertices.resize(65539);
	for (size_t i = 0; i < 3; ++i) {
		large.vertices[65536 + i] = triangleData.vertices[i];
	}
	large.indices = {65536, 65537, 65538};
	auto largeMesh = std::make_shared<Mesh>(context, allocator);
	largeMesh->Initialize(large.vertices, large.indices);
	Scene scene;
	scene.AddObject(largeMesh, 1);
	MeshRenderer renderer(context, descriptors);
	renderer.Initialize(swapChain);
	renderer.SetScene(scene);
	frame.Wait();
	frame.Prepare(FrameUniform{});
	probe.Render(renderer, frame, 0);
	Require(probe.HasGreenCenter(), "32-bit indexed device-address mesh did not render");

	const auto CUBE = MeshPrimitives::CreateCube();
	auto cube = std::make_shared<Mesh>(context, allocator);
	cube->Initialize(CUBE.vertices, CUBE.indices);
	scene.GetObject(0).GetTransform().SetPosition(glm::vec3(4, 0, 0));
	scene.AddObject(cube, 0).GetTransform().SetPosition(glm::vec3(0, 0, 0.5f));
	frame.Prepare(FrameUniform{});
	probe.Render(renderer, frame, 0);
	Require(probe.HasMulticolorCenter(), "second mesh or independent material did not render");
	scene.GetObject(1).SetMaterialIndex(1);
	frame.Prepare(FrameUniform{});
	probe.Render(renderer, frame, 0);
	Require(probe.HasGreenCenter(), "per-object material changes did not reach the shader");

	for (auto &vertex : triangleData.vertices) {
		vertex.normal = {0.70710678f, 0.70710678f, 0, 0};
	}
	Mesh attributes(context, allocator);
	attributes.Initialize(triangleData.vertices, triangleData.indices);
	AttributeDraw attributeDraw(context, descriptors, attributes);
	attributeDraw.Initialize(swapChain);
	frame.Prepare(FrameUniform{});
	probe.Render(attributeDraw, frame, 0);
	Require(probe.HasGreenCenter(), "vertex normals/UVs or nonuniform normal transformation are incorrect");
}
} // namespace VulkanRenderer
