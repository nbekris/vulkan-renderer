#include "Assets/GltfGeometry.h"
#include "ThirdParty/cgltf/cgltf.h"
#include "Math/Math.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <stdexcept>

namespace VulkanRenderer {
namespace {
const cgltf_accessor *Attribute(const cgltf_primitive &primitive, cgltf_attribute_type type, int index = 0) {
	for (size_t i = 0; i < primitive.attributes_count; ++i) {
		if (primitive.attributes[i].type == type && primitive.attributes[i].index == index) {
			return primitive.attributes[i].data;
		}
	}
	return nullptr;
}

std::vector<float> ReadFloats(const cgltf_accessor &accessor, size_t dimensions, size_t count) {
	if (accessor.count != count || cgltf_num_components(accessor.type) != dimensions
		|| count > std::numeric_limits<uint32_t>::max() || count > std::numeric_limits<size_t>::max() / dimensions) {
		throw std::runtime_error("glTF vertex attribute has an invalid shape");
	}
	std::vector<float> values(count * dimensions);
	if (cgltf_accessor_unpack_floats(&accessor, values.data(), values.size()) != values.size()) {
		throw std::runtime_error("cannot unpack glTF vertex attribute");
	}
	for (auto value : values) {
		if (!std::isfinite(value)) {
			throw std::runtime_error("glTF vertex attribute is not finite");
		}
	}
	return values;
}

void ExpandTopology(MeshData &data, cgltf_primitive_type type) {
	if (type == cgltf_primitive_type_triangles) {
		if (data.indices.size() % 3 != 0) {
			throw std::runtime_error("incomplete glTF triangles");
		}
		return;
	}
	if (type != cgltf_primitive_type_triangle_strip && type != cgltf_primitive_type_triangle_fan) {
		throw std::runtime_error("glTF primitive must be triangles, a triangle strip, or a triangle fan");
	}
	std::vector<uint32_t> indices;
	for (size_t i = 2; i < data.indices.size(); ++i) {
		uint32_t a = type == cgltf_primitive_type_triangle_fan ? data.indices[0] : data.indices[i - 2];
		uint32_t b = data.indices[i - 1];
		if (type == cgltf_primitive_type_triangle_strip && i % 2 != 0) {
			std::swap(a, b);
		}
		indices.insert(indices.end(), {a, b, data.indices[i]});
	}
	data.indices = std::move(indices);
}

void FlatNormals(MeshData &data) {
	MeshData flat;
	for (size_t i = 0; i < data.indices.size(); i += 3) {
		Vertex a = data.vertices.at(data.indices[i]), b = data.vertices.at(data.indices[i + 1]),
			   c = data.vertices.at(data.indices[i + 2]);
		const glm::vec3 A(a.position[0], a.position[1], a.position[2]);
		const glm::vec3 B(b.position[0], b.position[1], b.position[2]);
		const glm::vec3 C(c.position[0], c.position[1], c.position[2]);
		auto normal = glm::cross(B - A, C - A);
		normal = glm::length(normal) > 0.000001f ? glm::normalize(normal) : glm::vec3(0, 0, 1);
		a.normal = b.normal = c.normal = {normal.x, normal.y, normal.z, 0};
		const auto BASE = static_cast<uint32_t>(flat.vertices.size());
		flat.vertices.insert(flat.vertices.end(), {a, b, c});
		flat.indices.insert(flat.indices.end(), {BASE, BASE + 1, BASE + 2});
	}
	data = std::move(flat);
}
} // namespace

MeshData GltfGeometry::Read(const cgltf_primitive &primitive) {
	if (primitive.has_draco_mesh_compression || primitive.targets_count != 0) {
		throw std::runtime_error("Draco compression and morph targets are not supported");
	}
	const auto *position = Attribute(primitive, cgltf_attribute_type_position);
	if (!position || position->count == 0) {
		throw std::runtime_error("glTF mesh has no positions");
	}
	const auto POSITIONS = ReadFloats(*position, 3, position->count);
	MeshData data;
	data.vertices.resize(position->count);
	for (size_t i = 0; i < data.vertices.size(); ++i) {
		data.vertices[i].position = {POSITIONS[i * 3], POSITIONS[i * 3 + 1], POSITIONS[i * 3 + 2], 1};
	}
	const auto *normal = Attribute(primitive, cgltf_attribute_type_normal);
	if (normal) {
		const auto VALUES = ReadFloats(*normal, 3, position->count);
		for (size_t i = 0; i < data.vertices.size(); ++i) {
			const glm::vec3 NORMAL(VALUES[i * 3], VALUES[i * 3 + 1], VALUES[i * 3 + 2]);
			if (glm::length(NORMAL) < 0.000001f) {
				throw std::runtime_error("glTF normal is zero");
			}
			data.vertices[i].normal = {NORMAL.x, NORMAL.y, NORMAL.z, 0};
		}
	}
	const auto *color = Attribute(primitive, cgltf_attribute_type_color);
	if (color) {
		const size_t COMPONENTS = cgltf_num_components(color->type);
		if (COMPONENTS != 3 && COMPONENTS != 4) {
			throw std::runtime_error("invalid glTF color type");
		}
		const auto VALUES = ReadFloats(*color, COMPONENTS, position->count);
		for (size_t i = 0; i < data.vertices.size(); ++i) {
			for (size_t channel = 0; channel < COMPONENTS; ++channel) {
				data.vertices[i].color[channel] = VALUES[i * COMPONENTS + channel];
			}
		}
	}
	cgltf_texture_view view{};
	if (primitive.material) {
		view = primitive.material->pbr_metallic_roughness.base_color_texture;
	}
	const int UV_SET = view.has_transform && view.transform.has_texcoord ? view.transform.texcoord : view.texcoord;
	const auto *uv = Attribute(primitive, cgltf_attribute_type_texcoord, UV_SET);
	if (view.texture && !uv) {
		throw std::runtime_error("textured glTF primitive has no requested UV set");
	}
	if (uv) {
		const auto VALUES = ReadFloats(*uv, 2, position->count);
		for (size_t i = 0; i < data.vertices.size(); ++i) {
			glm::vec2 coordinate(VALUES[i * 2], VALUES[i * 2 + 1]);
			if (view.has_transform) {
				coordinate *= glm::vec2(view.transform.scale[0], view.transform.scale[1]);
				const float COSINE = std::cos(view.transform.rotation), SINE = std::sin(view.transform.rotation);
				coordinate = glm::vec2(COSINE * coordinate.x - SINE * coordinate.y,
									   SINE * coordinate.x + COSINE * coordinate.y)
							 + glm::vec2(view.transform.offset[0], view.transform.offset[1]);
			}
			data.vertices[i].uv = {coordinate.x, coordinate.y, 0, 0};
		}
	}
	if (primitive.indices) {
		if (primitive.indices->is_sparse) {
			throw std::runtime_error("sparse index accessors are not supported");
		}
		data.indices.resize(primitive.indices->count);
		if (cgltf_accessor_unpack_indices(primitive.indices, data.indices.data(), sizeof(uint32_t), data.indices.size())
			!= data.indices.size()) {
			throw std::runtime_error("cannot unpack glTF indices");
		}
	} else {
		data.indices.resize(data.vertices.size());
		std::iota(data.indices.begin(), data.indices.end(), 0u);
	}
	for (auto index : data.indices) {
		if (index >= data.vertices.size()) {
			throw std::runtime_error("glTF index is out of range");
		}
	}
	ExpandTopology(data, primitive.type);
	if (data.indices.empty()) {
		throw std::runtime_error("glTF primitive has no complete triangles");
	}
	if (!normal) {
		FlatNormals(data);
	}
	return data;
}
} // namespace VulkanRenderer
