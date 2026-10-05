#include "mesh-common.slang"
#include "brdf.slang"
#include "shadow-common.slang"

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
	if (MATERIAL.unlit != 0) {
		return float4(ALBEDO.rgb, MATERIAL.alphaCutoff < 0.0 ? 1.0 : ALBEDO.a);
	}
	const FrameUniform FRAME = frames[draw.frameIndex];
	const float3 NORMAL = SafeNormalize(input.worldNormal, float3(0, 0, 1));
	const float3 VIEW = SafeNormalize(FRAME.cameraPosition.xyz - input.worldPosition, NORMAL);
	const float3 LIGHT = SafeNormalize(FRAME.lightDirection.xyz, NORMAL);
	const float NORMAL_LIGHT = saturate(dot(NORMAL, LIGHT));
	const float3 BASE_COLOR = saturate(ALBEDO.rgb);
	const float3 DIRECT = EvaluateBrdf(BASE_COLOR, MATERIAL.metallic, MATERIAL.roughness, NORMAL, VIEW, LIGHT)
						  * FRAME.lightColor.rgb * FRAME.lightDirection.w * NORMAL_LIGHT
						  * ShadowVisibility(FRAME, input.worldPosition, NORMAL_LIGHT);
	// Ambient remains a diffuse approximation until environment-map lighting is implemented.
	const float3 RADIANCE = DIRECT + BASE_COLOR * FRAME.lightColor.w * (1.0 - MATERIAL.metallic);
	return float4(RADIANCE, MATERIAL.alphaCutoff < 0.0 ? 1.0 : ALBEDO.a);
}
