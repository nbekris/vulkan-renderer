#pragma once
#include "Assets/ModelData.h"
#include "Math/Bounds.h"
#include <filesystem>

namespace VulkanRenderer {
class Scene;
class SceneResources;

struct ImportStats {
	size_t meshes;
	size_t materials;
	size_t textures;
	size_t objects;
};

class GltfLoader {
public:
	GltfLoader() = delete;
	virtual ~GltfLoader() = default;
	GltfLoader(const GltfLoader &) = delete;
	GltfLoader &operator=(const GltfLoader &) = delete;
	static ModelData Read(const std::filesystem::path &path);
	static ImportStats Import(const std::filesystem::path &path, SceneResources &resources, Scene &scene);
	static void FrameScene(Scene &scene, float aspectRatio);
};
} // namespace VulkanRenderer
