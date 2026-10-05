// Shared CPU/GPU layouts. Compile with -matrix-layout-column-major to match GLM.
namespace VulkanRenderer {
static const uint MAX_FRAMES_IN_FLIGHT = 3;

struct Vertex {
	float4 position;
	float4 color;
	float4 normal;
	float4 uv;
};

struct FrameUniform {
	float4x4 transform;
	float4 lightDirection;
	float4 lightColor;
};

struct MaterialData {
	float4 tint;
	uint textureIndex;
	float alphaCutoff;
	uint unlit;
	uint padding;
};

struct DrawData {
	Vertex *vertices;
	uint frameIndex;
	uint materialIndex;
	float4x4 model;
};

struct VertexOutput {
	float4 position : SV_Position;
	[[vk::location(0)]] float4 color : COLOR0;
	[[vk::location(1)]] float3 worldNormal : NORMAL0;
	[[vk::location(2)]] float2 texCoord : TEXCOORD0;
};

[[vk::binding(0, 0)]] ConstantBuffer<FrameUniform> frames[MAX_FRAMES_IN_FLIGHT];
[[vk::push_constant]] ConstantBuffer<DrawData> draw;

} // namespace VulkanRenderer
