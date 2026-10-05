namespace VulkanRenderer {
[[vk::binding(3, 0)]] Sampler2D shadowMaps[MAX_FRAMES_IN_FLIGHT];

float ShadowVisibility(FrameUniform frame, float3 worldPosition, float normalLight) {
	if (frame.shadowParameters.x == 0.0) {
		return 1.0;
	}
	const float4 CLIP = mul(frame.lightTransform, float4(worldPosition, 1));
	const float3 PROJECTED = CLIP.xyz / CLIP.w;
	const float2 UV = PROJECTED.xy * 0.5 + 0.5;
	if (any(UV < 0.0) || any(UV > 1.0) || PROJECTED.z < 0.0 || PROJECTED.z > 1.0) {
		return 1.0;
	}
	const float BIAS = frame.shadowParameters.y + frame.shadowParameters.z * (1.0 - normalLight);
	float visibility = 0.0;
	for (int y = -1; y <= 1; ++y) {
		for (int x = -1; x <= 1; ++x) {
			const float DEPTH
				= shadowMaps[draw.frameIndex].SampleLevel(UV + float2(x, y) * frame.shadowParameters.w, 0).r;
			visibility += PROJECTED.z - BIAS <= DEPTH ? 1.0 : 0.0;
		}
	}
	return visibility / 9.0;
}
} // namespace VulkanRenderer
