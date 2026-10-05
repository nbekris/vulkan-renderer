#include "Rendering/Resources/SceneResources.h"
#include "Rendering/Resources/Mesh.h"
#include "Rendering/Resources/Material.h"
#include "Rendering/Resources/Texture.h"
#include "Rendering/RenderData.h"
#include "Rendering/Core/VulkanContext.h"
#include <stdexcept>

namespace VulkanRenderer {
SceneResources::SceneResources(const VulkanContext &context, const MemoryAllocator &allocator)
	: _context(context), _allocator(allocator) {
}

SceneResources::~SceneResources() noexcept {
	// Persistent assets may still be referenced by the final submitted frame.
	vkDeviceWaitIdle(_context.GetDevice());
}

void SceneResources::RequireMutable() const {
	if (!_initialized || _frozen) {
		throw std::logic_error("scene resources must be initialized and mutable before adding assets");
	}
}

const Material &SceneResources::GetMaterial(uint32_t index) const {
	return *_materials.at(index);
}

const Texture &SceneResources::GetTexture(uint32_t index) const {
	return *_textures.at(index);
}

void SceneResources::Initialize() {
	if (_initialized) {
		throw std::logic_error("scene resources already initialized");
	}
	_initialized = true;
	const std::array<uint8_t, 4> WHITE{255, 255, 255, 255};
	AddTexture({1, 1}, WHITE);
	AddMaterial(MaterialData{});
	MaterialData green;
	green.tint = {0, 1, 0, 1};
	AddMaterial(green);
}

ResourceCheckpoint SceneResources::GetCheckpoint() const {
	RequireMutable();
	return {_meshes.size(), _textures.size(), _materials.size()};
}

void SceneResources::Rollback(const ResourceCheckpoint &checkpoint) {
	RequireMutable();
	if (checkpoint.meshes > _meshes.size() || checkpoint.textures > _textures.size()
		|| checkpoint.materials > _materials.size() || checkpoint.textures < 1 || checkpoint.materials < 2) {
		throw std::invalid_argument("invalid resource checkpoint");
	}
	_materials.resize(checkpoint.materials);
	_textures.resize(checkpoint.textures);
	_meshes.resize(checkpoint.meshes);
}

void SceneResources::Freeze() {
	RequireMutable();
	_frozen = true;
}

std::shared_ptr<const Mesh> SceneResources::CreateMesh(const MeshData &data) {
	RequireMutable();
	auto mesh = std::make_shared<Mesh>(_context, _allocator);
	mesh->Initialize(data.vertices, data.indices);
	_meshes.push_back(mesh);
	return mesh;
}

uint32_t SceneResources::AddTexture(VkExtent2D extent, std::span<const uint8_t> pixels, TextureColorSpace colorSpace,
									const TextureSampling &sampling) {
	RequireMutable();
	if (_textures.size() >= TEXTURE_CAPACITY) {
		throw std::out_of_range("texture descriptor table capacity exceeded");
	}
	auto texture = std::make_unique<Texture>(_context, _allocator);
	texture->Initialize(extent, pixels, colorSpace, sampling);
	_textures.push_back(std::move(texture));
	return static_cast<uint32_t>(_textures.size() - 1);
}

uint32_t SceneResources::AddMaterial(const MaterialData &data) {
	RequireMutable();
	if (_materials.size() >= MATERIAL_CAPACITY || data.textureIndex >= _textures.size()) {
		throw std::out_of_range("material table is full or material references an unpopulated texture");
	}
	auto material = std::make_unique<Material>(_allocator);
	material->Initialize(data);
	_materials.push_back(std::move(material));
	return static_cast<uint32_t>(_materials.size() - 1);
}
} // namespace VulkanRenderer
