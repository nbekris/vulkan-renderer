#pragma once
#include "Assets/GltfLoader.h"

namespace VulkanRenderer {
/** Routes supported files to a CPU importer and commits assets atomically. */
class ModelLoader {
public:
	ModelLoader() = delete;
	virtual ~ModelLoader() = default;
	ModelLoader(const ModelLoader &) = delete;
	ModelLoader &operator=(const ModelLoader &) = delete;
	static std::filesystem::path ResolvePath(const std::filesystem::path &path);
	static ModelData Read(const std::filesystem::path &path);
	static ImportStats Import(const std::filesystem::path &path, SceneResources &resources, Scene &scene);
	static ImportStats Upload(const ModelData &model, SceneResources &resources, Scene &scene);
};
} // namespace VulkanRenderer
