#include "Assets/GltfLoader.h"
#include "Assets/GltfDocument.h"
#include "Assets/GltfGeometry.h"
#include "Assets/GltfImages.h"
#include "Scene/Scene.h"
#include "Rendering/Resources/SceneResources.h"
#include "Rendering/Resources/Mesh.h"
#include "ThirdParty/cgltf/cgltf.h"
#include <glm/gtc/type_ptr.hpp>
#include <unordered_map>
#include <functional>
#include <stdexcept>
#include <cmath>
#include <algorithm>
#include <limits>

namespace VulkanRenderer {
ModelData GltfLoader::Read(const std::filesystem::path &path) {
	GltfDocument document;
	document.Initialize(path);
	const auto &DATA = document.GetData();
	ModelData model;
	std::unordered_map<const cgltf_primitive *, size_t> meshes;
	std::unordered_map<const cgltf_material *, uint32_t> materials;
	std::unordered_map<const cgltf_texture *, uint32_t> textures;
	const auto MATERIAL = [&](const cgltf_material *source) {
		if (!source) {
			return NO_MODEL_RESOURCE;
		}
		if (materials.contains(source)) {
			return materials.at(source);
		}
		if (source->alpha_mode == cgltf_alpha_mode_blend || source->has_pbr_specular_glossiness) {
			throw std::runtime_error("transparent blending and specular/glossiness materials are not supported");
		}
		MaterialData material;
		std::copy_n(source->pbr_metallic_roughness.base_color_factor, 4, material.tint.begin());
		material.unlit = source->unlit ? 1u : 0u;
		material.alphaCutoff = source->alpha_mode == cgltf_alpha_mode_mask ? source->alpha_cutoff : -1.0f;
		material.textureIndex = NO_MODEL_RESOURCE;
		if (const auto *texture = source->pbr_metallic_roughness.base_color_texture.texture) {
			if (!textures.contains(texture)) {
				textures[texture] = static_cast<uint32_t>(model.textures.size());
				model.textures.push_back(GltfImages::Read(*texture, path.parent_path()));
			}
			material.textureIndex = textures.at(texture);
		}
		const auto INDEX = static_cast<uint32_t>(model.materials.size());
		materials[source] = INDEX;
		model.materials.push_back(material);
		return INDEX;
	};
	std::vector<const cgltf_node *> stack;
	const auto *selected = DATA.scene ? DATA.scene : (DATA.scenes_count ? &DATA.scenes[0] : nullptr);
	if (selected) {
		for (size_t i = 0; i < selected->nodes_count; ++i) {
			stack.push_back(selected->nodes[i]);
		}
	} else {
		for (size_t i = 0; i < DATA.nodes_count; ++i) {
			if (!DATA.nodes[i].parent) {
				stack.push_back(&DATA.nodes[i]);
			}
		}
	}
	std::unordered_map<const cgltf_node *, bool> visited;
	while (!stack.empty()) {
		const auto *node = stack.back();
		stack.pop_back();
		if (!node || visited.contains(node)) {
			throw std::runtime_error("glTF node hierarchy is cyclic or duplicated");
		}
		visited[node] = true;
		if (node->skin || node->has_mesh_gpu_instancing) {
			throw std::runtime_error("skinning and EXT_mesh_gpu_instancing are unsupported");
		}
		float values[16];
		cgltf_node_transform_world(node, values);
		const auto WORLD = glm::make_mat4(values);
		if (node->mesh) {
			for (size_t i = 0; i < node->mesh->primitives_count; ++i) {
				const auto *primitive = &node->mesh->primitives[i];
				if (!meshes.contains(primitive)) {
					meshes[primitive] = model.meshes.size();
					model.meshes.push_back(GltfGeometry::Read(*primitive));
				}
				model.instances.push_back({meshes.at(primitive), MATERIAL(primitive->material), WORLD});
			}
		}
		for (size_t i = 0; i < node->children_count; ++i) {
			stack.push_back(node->children[i]);
		}
	}
	if (model.instances.empty()) {
		throw std::runtime_error("selected glTF scene contains no renderable meshes");
	}
	// Validate transforms before allocating persistent GPU assets.
	for (const auto &instance : model.instances) {
		for (int column = 0; column < 4; ++column) {
			for (int row = 0; row < 4; ++row) {
				if (!std::isfinite(instance.transform[column][row])) {
					throw std::runtime_error("glTF node matrix is not finite");
				}
			}
		}
		if (glm::determinant(glm::mat3(instance.transform)) == 0) {
			throw std::runtime_error("glTF node matrix is singular");
		}
	}
	return model;
}

ImportStats GltfLoader::Import(const std::filesystem::path &path, SceneResources &resources, Scene &scene) {
	const auto CHECKPOINT = resources.GetCheckpoint();
	const size_t OBJECT_COUNT = scene.GetObjects().size();
	const auto MODEL = Read(path);
	if (MODEL.materials.size() > MATERIAL_CAPACITY - resources.GetMaterialCount()
		|| MODEL.textures.size() > TEXTURE_CAPACITY - resources.GetTextureCount()) {
		throw std::runtime_error("glTF asset exceeds available material or texture table capacity");
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

void GltfLoader::FrameScene(Scene &scene, float aspectRatio) {
	if (scene.GetObjects().empty() || !std::isfinite(aspectRatio) || aspectRatio <= 0) {
		throw std::invalid_argument("framing requires a nonempty scene and positive aspect ratio");
	}
	Bounds bounds{glm::vec3(std::numeric_limits<float>::max()), glm::vec3(std::numeric_limits<float>::lowest())};
	for (const auto &object : scene.GetObjects()) {
		const auto &LOCAL = object->GetMesh().GetBounds();
		for (unsigned i = 0; i < 8; ++i) {
			const glm::vec3 CORNER(i & 1 ? LOCAL.maximum.x : LOCAL.minimum.x, i & 2 ? LOCAL.maximum.y : LOCAL.minimum.y,
								   i & 4 ? LOCAL.maximum.z : LOCAL.minimum.z);
			const auto POINT = glm::vec3(object->GetModelMatrix() * glm::vec4(CORNER, 1));
			bounds.minimum = glm::min(bounds.minimum, POINT);
			bounds.maximum = glm::max(bounds.maximum, POINT);
		}
	}
	const auto CENTER = (bounds.minimum + bounds.maximum) * 0.5f;
	const float RADIUS = std::max(0.01f, glm::length(bounds.maximum - bounds.minimum) * 0.5f);
	const float HALF_FOV = std::min(glm::radians(30.0f), std::atan(std::tan(glm::radians(30.0f)) * aspectRatio));
	const float DISTANCE = RADIUS / std::sin(HALF_FOV) * 1.1f;
	scene.GetCamera().SetPosition(CENTER + glm::vec3(0, 0, DISTANCE));
	scene.GetCamera().SetClipPlanes(std::max(0.001f, RADIUS * 0.001f), std::max(100.0f, DISTANCE + RADIUS * 8));
}
} // namespace VulkanRenderer
