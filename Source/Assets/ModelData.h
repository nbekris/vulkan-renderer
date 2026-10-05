#pragma once
#include "Assets/MeshData.h"
#include "Rendering/Resources/Material.h"
#include "Assets/TexturePixels.h"
#include "Rendering/Resources/TextureSampling.h"
#include "Math/Math.h"
#include <limits>

namespace VulkanRenderer {
inline constexpr uint32_t NO_MODEL_RESOURCE = std::numeric_limits<uint32_t>::max();

struct ModelTexture {
	TextureMip image;
	TextureSampling sampling;
};

struct ModelInstance {
	size_t mesh;
	uint32_t material;
	glm::mat4 transform{1};
};

struct ModelData {
	std::vector<MeshData> meshes;
	std::vector<ModelTexture> textures;
	std::vector<MaterialData> materials;
	std::vector<ModelInstance> instances;
};
} // namespace VulkanRenderer
