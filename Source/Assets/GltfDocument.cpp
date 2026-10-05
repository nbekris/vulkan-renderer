#include "Assets/GltfDocument.h"
#include "ThirdParty/cgltf/cgltf.h"
#include <stdexcept>
#include <string>

namespace VulkanRenderer {
GltfDocument::~GltfDocument() noexcept {
	cgltf_free(_data);
}

void GltfDocument::Initialize(const std::filesystem::path &path) {
	if (_data) {
		throw std::logic_error("glTF document already initialized");
	}
	cgltf_options options{};
	const auto FILE = path.string();
	const auto CHECK = [&FILE](cgltf_result result, const char *operation) {
		if (result != cgltf_result_success) {
			throw std::runtime_error(std::string(operation) + " for " + FILE + " (cgltf result "
									 + std::to_string(result) + ")");
		}
	};
	CHECK(cgltf_parse_file(&options, FILE.c_str(), &_data), "failed to parse glTF");
	CHECK(cgltf_load_buffers(&options, _data, FILE.c_str()), "failed to load glTF buffers");
	CHECK(cgltf_validate(_data), "invalid glTF data");
	if (!_data->asset.version || std::string(_data->asset.version) != "2.0") {
		throw std::runtime_error("only glTF 2.0 is supported");
	}
	for (size_t i = 0; i < _data->extensions_required_count; ++i) {
		const std::string EXTENSION = _data->extensions_required[i];
		if (EXTENSION != "KHR_texture_transform" && EXTENSION != "KHR_materials_unlit"
			&& EXTENSION != "KHR_mesh_quantization") {
			throw std::runtime_error("unsupported required glTF extension: " + EXTENSION);
		}
	}
	for (size_t i = 0; i < _data->buffer_views_count; ++i) {
		if (_data->buffer_views[i].has_meshopt_compression) {
			throw std::runtime_error("meshopt-compressed buffers are not supported");
		}
	}
}
} // namespace VulkanRenderer
