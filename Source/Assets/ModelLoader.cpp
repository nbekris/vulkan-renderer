#include "Assets/ModelLoader.h"
#include "Assets/AssimpLoader.h"
#include "Scene/Scene.h"
#include "Rendering/Resources/SceneResources.h"
#include "Rendering/Resources/Mesh.h"
#include <assimp/Importer.hpp>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <stdexcept>

namespace VulkanRenderer {
namespace {
std::string Extension(const std::filesystem::path &path) {
	auto extension = path.extension().string();
	std::transform(extension.begin(), extension.end(), extension.begin(),
				   [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
	return extension;
}

void Validate(const ModelData &model) {
	if (model.instances.empty()) {
		throw std::runtime_error("model contains no renderable objects");
	}
	for (const auto &instance : model.instances) {
		if (instance.mesh >= model.meshes.size()
			|| (instance.material != NO_MODEL_RESOURCE && instance.material >= model.materials.size())) {
			throw std::runtime_error("model contains invalid resource references");
		}
		for (int column = 0; column < 4; ++column) {
			for (int row = 0; row < 4; ++row) {
				if (!std::isfinite(instance.transform[column][row])) {
					throw std::runtime_error("model transform is not finite");
				}
			}
		}
		if (glm::determinant(glm::mat3(instance.transform)) == 0) {
			throw std::runtime_error("model transform is singular");
		}
	}
}
} // namespace

std::filesystem::path ModelLoader::ResolvePath(const std::filesystem::path &path) {
	if (std::filesystem::is_regular_file(path)) {
		return path;
	}
	if (!std::filesystem::is_directory(path)) {
		throw std::runtime_error("model path does not exist: " + path.string());
	}
	Assimp::Importer importer;
	std::vector<std::filesystem::path> candidates;
	for (const auto &entry : std::filesystem::directory_iterator(path)) {
		const auto EXTENSION = Extension(entry.path());
		// .bin is normally a glTF sidecar, never an automatic model candidate.
		if (entry.is_regular_file() && EXTENSION != ".bin" && importer.IsExtensionSupported(EXTENSION)) {
			candidates.push_back(entry.path());
		}
	}
	if (candidates.size() != 1) {
		throw std::runtime_error("model folder must contain exactly one supported model file; found "
								 + std::to_string(candidates.size()) + " in " + path.string()
								 + ". Use --model with an explicit file path.");
	}
	return candidates.front();
}

ModelData ModelLoader::Read(const std::filesystem::path &path) {
	const auto FILE = ResolvePath(path);
	const auto EXTENSION = Extension(FILE);
	auto model = EXTENSION == ".gltf" || EXTENSION == ".glb" ? GltfLoader::Read(FILE) : AssimpLoader::Read(FILE);
	Validate(model);
	return model;
}

ImportStats ModelLoader::Import(const std::filesystem::path &path, SceneResources &resources, Scene &scene) {
	return Upload(Read(path), resources, scene);
}

ImportStats ModelLoader::Upload(const ModelData &MODEL, SceneResources &resources, Scene &scene) {
	Validate(MODEL);
	const auto CHECKPOINT = resources.GetCheckpoint();
	const size_t OBJECT_COUNT = scene.GetObjects().size();
	if (MODEL.materials.size() > MATERIAL_CAPACITY - resources.GetMaterialCount()
		|| MODEL.textures.size() > TEXTURE_CAPACITY - resources.GetTextureCount()) {
		throw std::runtime_error("model exceeds available material or texture table capacity");
	}
	try {
		std::vector<uint32_t> textures, materials;
		std::vector<std::shared_ptr<const Mesh>> meshes;
		for (const auto &texture : MODEL.textures) {
			textures.push_back(resources.AddTexture(texture.image.extent, texture.image.pixels, TextureColorSpace::Srgb,
													texture.sampling));
		}
		for (auto material : MODEL.materials) {
			material.textureIndex = material.textureIndex == NO_MODEL_RESOURCE ? 0 : textures.at(material.textureIndex);
			materials.push_back(resources.AddMaterial(material));
		}
		for (const auto &mesh : MODEL.meshes) {
			meshes.push_back(resources.CreateMesh(mesh));
		}
		for (const auto &instance : MODEL.instances) {
			scene.AddObject(meshes.at(instance.mesh),
							instance.material == NO_MODEL_RESOURCE ? 0 : materials.at(instance.material),
							instance.transform);
		}
	} catch (...) {
		scene.TruncateObjects(OBJECT_COUNT);
		resources.Rollback(CHECKPOINT);
		throw;
	}
	return {MODEL.meshes.size(), MODEL.materials.size(), MODEL.textures.size(), MODEL.instances.size()};
}

} // namespace VulkanRenderer
