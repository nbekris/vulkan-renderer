#include "mesh-common.slang"

using namespace VulkanRenderer;

namespace VulkanRenderer {
float3 TransformNormal(float3 normal, float3x3 model) {
	// Cofactor rows / DETERMINANT give inverse-transpose, including mirrored and nonuniform scale.
	const float3 COFACTOR_X = cross(model[1], model[2]);
	const float3 COFACTOR_Y = cross(model[2], model[0]);
	const float3 COFACTOR_Z = cross(model[0], model[1]);
	const float DETERMINANT = dot(model[0], COFACTOR_X);
	return normalize(mul(float3x3(COFACTOR_X, COFACTOR_Y, COFACTOR_Z), normal) / DETERMINANT);
}

} // namespace VulkanRenderer

[shader("vertex")] VertexOutput Main(uint vertexIndex : SV_VulkanVertexID) {
	const Vertex VERTEX = draw.vertices[vertexIndex];
	VertexOutput output;
	output.position = mul(frames[draw.frameIndex].transform, mul(draw.model, VERTEX.position));
	output.color = VERTEX.color;
	output.worldNormal = TransformNormal(VERTEX.normal.xyz, (float3x3)draw.model);
	output.texCoord = VERTEX.uv.xy;
	return output;
}
