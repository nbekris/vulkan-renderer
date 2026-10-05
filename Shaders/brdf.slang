namespace VulkanRenderer {
static const float PI = 3.14159265358979323846;
static const float MIN_ROUGHNESS = 0.045;

float DistributionGgx(float normalHalf, float roughness) {
	const float ALPHA = roughness * roughness;
	const float ALPHA_SQUARED = ALPHA * ALPHA;
	const float COSINE_SQUARED = normalHalf * normalHalf;
	const float DENOMINATOR = (1.0 - COSINE_SQUARED) + COSINE_SQUARED * ALPHA_SQUARED;
	return ALPHA_SQUARED / max(PI * DENOMINATOR * DENOMINATOR, 1e-12);
}

float VisibilitySmithGgx(float normalView, float normalLight, float roughness) {
	const float ALPHA = roughness * roughness;
	const float ALPHA_SQUARED = ALPHA * ALPHA;
	const float VIEW = normalLight * sqrt(normalView * normalView * (1.0 - ALPHA_SQUARED) + ALPHA_SQUARED);
	const float LIGHT = normalView * sqrt(normalLight * normalLight * (1.0 - ALPHA_SQUARED) + ALPHA_SQUARED);
	return 0.5 / max(VIEW + LIGHT, 1e-6);
}

float3 FresnelSchlick(float viewHalf, float3 reflectance) {
	const float GRAZING = pow(1.0 - viewHalf, 5.0);
	return reflectance + (float3(1.0) - reflectance) * GRAZING;
}

// Cook-Torrance BRDF: GGX distribution, correlated Smith visibility, Schlick Fresnel.
// Returns reflectance per steradian; the caller supplies radiance and the light cosine.
float3 EvaluateBrdf(float3 albedo, float metallic, float roughness, float3 normal, float3 view, float3 light) {
	const float NORMAL_VIEW = saturate(dot(normal, view));
	const float NORMAL_LIGHT = saturate(dot(normal, light));
	if (NORMAL_VIEW <= 0.0 || NORMAL_LIGHT <= 0.0) {
		return float3(0.0);
	}
	const float3 HALF = normalize(view + light);
	const float NORMAL_HALF = saturate(dot(normal, HALF));
	const float VIEW_HALF = saturate(dot(view, HALF));
	const float ROUGHNESS = clamp(roughness, MIN_ROUGHNESS, 1.0);
	const float3 REFLECTANCE = lerp(float3(0.04), albedo, metallic);
	const float3 FRESNEL = FresnelSchlick(VIEW_HALF, REFLECTANCE);
	const float DISTRIBUTION = DistributionGgx(NORMAL_HALF, ROUGHNESS);
	const float VISIBILITY = VisibilitySmithGgx(NORMAL_VIEW, NORMAL_LIGHT, ROUGHNESS);
	const float3 DIFFUSE_WEIGHT = (float3(1.0) - FRESNEL) * (1.0 - metallic);
	return DIFFUSE_WEIGHT * albedo / PI + DISTRIBUTION * VISIBILITY * FRESNEL;
}

float3 SafeNormalize(float3 direction, float3 fallback) {
	const float LENGTH_SQUARED = dot(direction, direction);
	if (LENGTH_SQUARED <= 1e-12) {
		return fallback;
	}
	return direction * rsqrt(LENGTH_SQUARED);
}
} // namespace VulkanRenderer
