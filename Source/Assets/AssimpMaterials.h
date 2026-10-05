#pragma once
#include "Assets/ModelData.h"
#include <filesystem>
struct aiScene;

namespace VulkanRenderer {
class AssimpMaterials {
public:
	AssimpMaterials() = delete;
	virtual ~AssimpMaterials() = default;
	AssimpMaterials(const AssimpMaterials &) = delete;
	AssimpMaterials &operator=(const AssimpMaterials &) = delete;
	static void Read(const aiScene &scene, const std::filesystem::path &directory, ModelData &model);
};
} // namespace VulkanRenderer
