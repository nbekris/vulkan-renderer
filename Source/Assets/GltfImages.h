#pragma once
#include "Assets/ModelData.h"
#include <filesystem>
struct cgltf_texture;

namespace VulkanRenderer {
class GltfImages {
public:
	GltfImages() = delete;
	virtual ~GltfImages() = default;
	GltfImages(const GltfImages &) = delete;
	GltfImages &operator=(const GltfImages &) = delete;
	static ModelTexture Read(const cgltf_texture &texture, const std::filesystem::path &directory);
};
} // namespace VulkanRenderer
