#pragma once
#include "Assets/TexturePixels.h"
#include "Rendering/Resources/TextureSampling.h"
#include <memory>
#include <vector>

namespace VulkanRenderer {
class VulkanContext;
class MemoryAllocator;
class Mesh;
struct MeshData;
class Material;
struct MaterialData;
class Texture;

struct ResourceCheckpoint {
	size_t meshes;
	size_t textures;
	size_t materials;
};

/** Persistent mesh, material, and texture ownership. Freeze the table before any descriptors reference it. */
class SceneResources {
public:
	SceneResources(const VulkanContext &context, const MemoryAllocator &allocator);
	virtual ~SceneResources() noexcept;
	SceneResources(const SceneResources &) = delete;
	SceneResources &operator=(const SceneResources &) = delete;

	bool IsFrozen() const noexcept { return _frozen; }

	uint32_t GetMaterialCount() const noexcept { return static_cast<uint32_t>(_materials.size()); }

	uint32_t GetTextureCount() const noexcept { return static_cast<uint32_t>(_textures.size()); }

	const Material &GetMaterial(uint32_t index) const;
	const Texture &GetTexture(uint32_t index) const;
	void Initialize();
	ResourceCheckpoint GetCheckpoint() const;
	void Rollback(const ResourceCheckpoint &checkpoint);
	void Freeze();
	std::shared_ptr<const Mesh> CreateMesh(const MeshData &data);
	uint32_t AddTexture(VkExtent2D extent, std::span<const uint8_t> pixels,
						TextureColorSpace colorSpace = TextureColorSpace::Srgb, const TextureSampling &sampling = {});
	uint32_t AddMaterial(const MaterialData &data);

private:
	void RequireMutable() const;
	const VulkanContext &_context;
	const MemoryAllocator &_allocator;
	bool _initialized = false;
	bool _frozen = false;
	std::vector<std::shared_ptr<const Mesh>> _meshes;
	std::vector<std::unique_ptr<Texture>> _textures;
	std::vector<std::unique_ptr<Material>> _materials;
};
} // namespace VulkanRenderer
