#include "Assets/ImageDecoder.h"
#include "ThirdParty/stb/stb_image.h"
#include <memory>
#include <limits>
#include <stdexcept>
#include <string>

namespace VulkanRenderer {
TextureMip ImageDecoder::Decode(std::span<const uint8_t> encoded) {
	if (encoded.empty() || encoded.size() > static_cast<size_t>(std::numeric_limits<int>::max())) {
		throw std::invalid_argument("invalid encoded image size");
	}
	int width = 0, height = 0, channels = 0;
	std::unique_ptr<stbi_uc, decltype(&stbi_image_free)> pixels(
		stbi_load_from_memory(encoded.data(), static_cast<int>(encoded.size()), &width, &height, &channels, 4),
		stbi_image_free);
	if (!pixels || width <= 0 || height <= 0) {
		throw std::runtime_error(std::string("PNG/JPEG image decode failed: ") + stbi_failure_reason());
	}
	const size_t BYTES = static_cast<size_t>(width) * static_cast<size_t>(height) * 4;
	return {{static_cast<uint32_t>(width), static_cast<uint32_t>(height)}, {pixels.get(), pixels.get() + BYTES}};
}
} // namespace VulkanRenderer
