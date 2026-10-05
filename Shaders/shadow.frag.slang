#include "mesh-common.slang"
using namespace VulkanRenderer;

namespace VulkanRenderer {
[[vk::binding(1, 0)]] StructuredBuffer<MaterialData> materials[];
[[vk::binding(2, 0)]] Sampler2D textures[];
} // namespace VulkanRenderer
[shader("fragment")] void Main(VertexOutput input) {
	const MaterialData MATERIAL = materials[draw.materialIndex][0];
	if (MATERIAL.alphaCutoff >= 0.0) {
		const float ALPHA = textures[NonUniformResourceIndex(MATERIAL.textureIndex)].Sample(input.texCoord).a
							* input.color.a * MATERIAL.tint.a;
		if (ALPHA < MATERIAL.alphaCutoff) {
			discard;
		}
	}
}
