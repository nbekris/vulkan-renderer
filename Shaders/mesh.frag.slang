#include "mesh-common.slang"

using namespace VulkanRenderer;

namespace VulkanRenderer {
[[vk::binding(1, 0)]] StructuredBuffer<MaterialData> materials[];
[[vk::binding(2, 0)]] Sampler2D textures[];

} // namespace VulkanRenderer

[shader("fragment")] float4 Main(VertexOutput input) : SV_Target0 {
	const MaterialData MATERIAL = materials[draw.materialIndex][0];
	const float4 ALBEDO
		= textures[NonUniformResourceIndex(MATERIAL.textureIndex)].Sample(input.texCoord) * input.color * MATERIAL.tint;
	if (MATERIAL.alphaCutoff >= 0.0 && ALBEDO.a < MATERIAL.alphaCutoff) {
		discard;
	}
	const float4 LIGHT = frames[draw.frameIndex].lightDirection;
	const float4 COLOR = frames[draw.frameIndex].lightColor;
	const float DIFFUSE = max(dot(normalize(input.worldNormal), normalize(LIGHT.xyz)), 0.0);
	float3 illumination = COLOR.www + COLOR.rgb * LIGHT.w * DIFFUSE;
	if (MATERIAL.unlit != 0) {
		illumination = float3(1.0);
	}
	return float4(ALBEDO.rgb * illumination, MATERIAL.alphaCutoff < 0.0 ? 1.0 : ALBEDO.a);
}
