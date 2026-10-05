#pragma once
#include <filesystem>
struct cgltf_data;

namespace VulkanRenderer {
/** Owns a parsed and validated glTF/GLB document and its loaded buffers. */
class GltfDocument {
public:
	GltfDocument() = default;
	virtual ~GltfDocument() noexcept;
	GltfDocument(const GltfDocument &) = delete;
	GltfDocument &operator=(const GltfDocument &) = delete;

	const cgltf_data &GetData() const noexcept { return *_data; }

	void Initialize(const std::filesystem::path &path);

private:
	cgltf_data *_data = nullptr;
};
} // namespace VulkanRenderer
