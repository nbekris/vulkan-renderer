#include "Assets/AssimpLoader.h"
#include "Assets/AssimpMaterials.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <cmath>
#include <stdexcept>
#include <utility>
#include <charconv>
#include <fstream>
#include <optional>
#include <string_view>
#include <limits>

namespace VulkanRenderer {
namespace {
// Preserve indexed connectivity for simple ASCII triangle PLYs. Assimp's
// smooth-normal pass performs spatial searches on this large reconstruction.
std::optional<ModelData> ReadIndexedPly(const std::filesystem::path &path) {
	if (path.extension() != ".ply") {
		return std::nullopt;
	}
	std::ifstream input(path);
	std::string line;
	if (!std::getline(input, line) || line != "ply"
		|| !std::getline(input, line) || line != "format ascii 1.0") {
		return std::nullopt;
	}
	size_t vertexCount = 0, faceCount = 0;
	std::vector<std::string> properties;
	bool ended = false;
	while (std::getline(input, line)) {
		if (line == "end_header") {
			ended = true;
			break;
		}
		if (line.starts_with("element vertex ")) {
			vertexCount = std::stoull(line.substr(15));
		} else if (line.starts_with("element face ")) {
			faceCount = std::stoull(line.substr(13));
		} else if (line.starts_with("property ")) {
			properties.push_back(line);
		} else if (!line.starts_with("comment ") && !line.starts_with("obj_info ")) {
			return std::nullopt;
		}
	}
	if (!ended || properties != std::vector<std::string>{"property float x", "property float y",
		"property float z", "property list uchar int vertex_indices"}) {
		return std::nullopt;
	}
	if (!vertexCount || !faceCount || vertexCount > std::numeric_limits<uint32_t>::max()
		|| faceCount > std::numeric_limits<uint32_t>::max() / 3) {
		throw std::runtime_error("PLY has invalid geometry counts");
	}
	auto readValue = [](std::string_view &remaining, auto &value) {
		const auto START = remaining.find_first_not_of(" \t\r");
		if (START == std::string_view::npos) {
			throw std::runtime_error("PLY record is incomplete");
		}
		remaining.remove_prefix(START);
		const auto RESULT = std::from_chars(remaining.data(), remaining.data() + remaining.size(), value);
		if (RESULT.ec != std::errc()) {
			throw std::runtime_error("PLY record contains an invalid number");
		}
		remaining.remove_prefix(static_cast<size_t>(RESULT.ptr - remaining.data()));
	};
	MeshData mesh;
	mesh.vertices.resize(vertexCount);
	mesh.indices.reserve(faceCount * 3);
	for (auto &vertex : mesh.vertices) {
		if (!std::getline(input, line)) {
			throw std::runtime_error("PLY vertex data is truncated");
		}
		std::string_view remaining(line);
		for (int axis = 0; axis < 3; ++axis) {
			readValue(remaining, vertex.position[axis]);
			if (!std::isfinite(vertex.position[axis])) {
				throw std::runtime_error("PLY position is not finite");
			}
		}
		vertex.position[3] = 1;
		vertex.normal = {0, 0, 0, 0};
	}
	for (size_t face = 0; face < faceCount; ++face) {
		if (!std::getline(input, line)) {
			throw std::runtime_error("PLY face data is truncated");
		}
		std::string_view remaining(line);
		uint32_t count = 0;
		readValue(remaining, count);
		if (count != 3) {
			return std::nullopt; // Other polygon layouts use Assimp's triangulator.
		}
		uint32_t indices[3];
		glm::vec3 points[3];
		for (int corner = 0; corner < 3; ++corner) {
			readValue(remaining, indices[corner]);
			if (indices[corner] >= vertexCount) {
				throw std::runtime_error("PLY face index is outside the vertex array");
			}
			const auto &P = mesh.vertices[indices[corner]].position;
			points[corner] = {P[0], P[1], P[2]};
			mesh.indices.push_back(indices[corner]);
		}
		const auto NORMAL = glm::cross(points[1] - points[0], points[2] - points[0]);
		for (auto index : indices) {
			for (int axis = 0; axis < 3; ++axis) {
				mesh.vertices[index].normal[axis] += NORMAL[axis];
			}
		}
	}
	for (auto &vertex : mesh.vertices) {
		auto &n = vertex.normal;
		const glm::vec3 NORMAL(n[0], n[1], n[2]);
		const float LENGTH = glm::length(NORMAL);
		const auto UNIT = LENGTH > 0 ? NORMAL / LENGTH : glm::vec3(0, 1, 0);
		n = {UNIT.x, UNIT.y, UNIT.z, 0};
	}
	ModelData model;
	model.meshes.push_back(std::move(mesh));
	model.instances.push_back({0, NO_MODEL_RESOURCE, glm::mat4(1)});
	return model;
}

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
	if (auto indexed = ReadIndexedPly(path)) {
		return std::move(*indexed);
	}
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
