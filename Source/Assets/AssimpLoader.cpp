#include "Assets/AssimpLoader.h"
#include "Assets/AssimpMaterials.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace VulkanRenderer {
namespace {
glm::mat4 Matrix(const aiMatrix4x4 &matrix) {
	return {matrix.a1, matrix.b1, matrix.c1, matrix.d1, matrix.a2, matrix.b2, matrix.c2, matrix.d2,
			matrix.a3, matrix.b3, matrix.c3, matrix.d3, matrix.a4, matrix.b4, matrix.c4, matrix.d4};
}

MeshData Geometry(const aiMesh &mesh) {
	if (mesh.HasBones() || mesh.mNumAnimMeshes) {
		throw std::runtime_error("Assimp import: skinning and morph targets are unsupported");
	}
	if (!mesh.HasPositions() || !mesh.HasNormals() || !mesh.mNumFaces) {
		throw std::runtime_error("Assimp import: mesh has no triangle geometry or normals");
	}
	MeshData result;
	result.vertices.reserve(mesh.mNumVertices);
	for (unsigned i = 0; i < mesh.mNumVertices; ++i) {
		Vertex vertex;
		const auto &POSITION = mesh.mVertices[i];
		const auto &NORMAL = mesh.mNormals[i];
		vertex.position = {POSITION.x, POSITION.y, POSITION.z, 1};
		vertex.normal = {NORMAL.x, NORMAL.y, NORMAL.z, 0};
		if (mesh.HasTextureCoords(0)) {
			vertex.uv = {mesh.mTextureCoords[0][i].x, mesh.mTextureCoords[0][i].y, 0, 0};
		}
		if (mesh.HasVertexColors(0)) {
			const auto &COLOR = mesh.mColors[0][i];
			vertex.color = {COLOR.r, COLOR.g, COLOR.b, COLOR.a};
		}
		for (const auto &field : {vertex.position, vertex.normal, vertex.uv, vertex.color}) {
			for (float value : field) {
				if (!std::isfinite(value)) {
					throw std::runtime_error("Assimp import: vertex data is not finite");
				}
			}
		}
		result.vertices.push_back(vertex);
	}
	for (unsigned i = 0; i < mesh.mNumFaces; ++i) {
		const auto &FACE = mesh.mFaces[i];
		if (FACE.mNumIndices != 3) {
			throw std::runtime_error("Assimp import: only triangle geometry is supported");
		}
		for (unsigned j = 0; j < 3; ++j) {
			if (FACE.mIndices[j] >= mesh.mNumVertices) {
				throw std::runtime_error("Assimp import: mesh index is out of range");
			}
			result.indices.push_back(FACE.mIndices[j]);
		}
	}
	return result;
}
} // namespace

ModelData AssimpLoader::Read(const std::filesystem::path &path) {
	Assimp::Importer importer;
	const auto *scene = importer.ReadFile(path.string(), aiProcess_Triangulate | aiProcess_JoinIdenticalVertices
															 | aiProcess_GenSmoothNormals
															 | aiProcess_ValidateDataStructure | aiProcess_FlipUVs);
	if (!scene || !scene->mRootNode || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) {
		throw std::runtime_error("Assimp failed to import " + path.string() + ": " + importer.GetErrorString());
	}
	if (scene->HasAnimations()) {
		throw std::runtime_error("Assimp import: animated scenes are unsupported; export a static model");
	}
	ModelData model;
	AssimpMaterials::Read(*scene, path.parent_path(), model);
	for (unsigned i = 0; i < scene->mNumMeshes; ++i) {
		model.meshes.push_back(Geometry(*scene->mMeshes[i]));
	}
	std::vector<std::pair<const aiNode *, glm::mat4>> pending{{scene->mRootNode, glm::mat4(1)}};
	while (!pending.empty()) {
		const auto [NODE, PARENT] = pending.back();
		pending.pop_back();
		const auto WORLD = PARENT * Matrix(NODE->mTransformation);
		for (unsigned i = 0; i < NODE->mNumMeshes; ++i) {
			const auto INDEX = NODE->mMeshes[i];
			if (INDEX >= scene->mNumMeshes || scene->mMeshes[INDEX]->mMaterialIndex >= model.materials.size()) {
				throw std::runtime_error("Assimp import: invalid mesh or material reference");
			}
			model.instances.push_back({INDEX, scene->mMeshes[INDEX]->mMaterialIndex, WORLD});
		}
		for (unsigned i = 0; i < NODE->mNumChildren; ++i) {
			pending.emplace_back(NODE->mChildren[i], WORLD);
		}
	}
	if (model.instances.empty()) {
		throw std::runtime_error("Assimp import: scene contains no renderable meshes");
	}
	return model;
}
} // namespace VulkanRenderer
