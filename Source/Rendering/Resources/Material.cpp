#include "Rendering/Resources/Material.h"
#include <cmath>
#include <stdexcept>

namespace VulkanRenderer {
Material::Material(const MemoryAllocator &allocator) : _buffer(allocator) {
}

void Material::Initialize(const MaterialData &data) {
	for (auto value : data.tint) {
		if (!std::isfinite(value) || value < 0) {
			throw std::invalid_argument("material tint must be finite and nonnegative");
		}
	}
	if (!std::isfinite(data.alphaCutoff) || (data.alphaCutoff != -1 && (data.alphaCutoff < 0 || data.alphaCutoff > 1))
		|| data.unlit > 1) {
		throw std::invalid_argument("invalid material alpha cutoff or unlit flag");
	}
	_buffer.Initialize(sizeof(data), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
	_buffer.Upload(&data, sizeof(data));
}
} // namespace VulkanRenderer
