#include "mesh-common.slang"
using namespace VulkanRenderer;
[shader("vertex")] VertexOutput Main(uint vertexIndex : SV_VulkanVertexID) {
	const Vertex VERTEX = draw.vertices[vertexIndex];
	VertexOutput output;
	const float4 WORLD = mul(draw.model, VERTEX.position);
	output.position = mul(frames[draw.frameIndex].lightTransform, WORLD);
	output.worldPosition = WORLD.xyz;
	output.worldNormal = float3(0, 1, 0);
	output.color = VERTEX.color;
	output.texCoord = VERTEX.uv.xy;
	return output;
}
