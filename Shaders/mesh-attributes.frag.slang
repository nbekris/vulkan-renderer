#include "mesh-common.slang"

using namespace VulkanRenderer;

[shader("fragment")] float4 Main(VertexOutput input) : SV_Target0 {
	const bool NORMAL_MATCHES
		= abs(input.worldNormal.x - 0.4472136) < 0.01 && abs(input.worldNormal.y - 0.8944272) < 0.01;
	const bool UV_MATCHES = all(abs(input.texCoord - float2(0.5)) < float2(0.02));
	return NORMAL_MATCHES && UV_MATCHES ? float4(0, 1, 0, 1) : float4(1, 0, 0, 1);
}
