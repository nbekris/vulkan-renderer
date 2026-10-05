#pragma once
#include "Rendering/Memory/AllocatedBuffer.h"
#include <array>
#include <cstdint>
#include <cstddef>

namespace VulkanRenderer {
struct alignas(16) MaterialData {
	std::array<float, 4> tint = {1, 1, 1, 1};
	uint32_t textureIndex = 0;
	float alphaCutoff = -1;
	uint32_t unlit = 0;
	uint32_t padding = 0;
};

static_assert(sizeof(MaterialData) == 32);
static_assert(offsetof(MaterialData, textureIndex) == 16);

/** Owns an immutable material buffer independently of presentation descriptor sets. */
class Material {
public:
	explicit Material(const MemoryAllocator &allocator);
	virtual ~Material() = default;
	Material(const Material &) = delete;
	Material &operator=(const Material &) = delete;

	VkBuffer GetBuffer() const noexcept { return _buffer.GetHandle(); }

	void Initialize(const MaterialData &data);

private:
	AllocatedBuffer _buffer;
};
} // namespace VulkanRenderer
