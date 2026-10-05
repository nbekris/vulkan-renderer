#pragma once
#include "Assets/TexturePixels.h"

namespace VulkanRenderer {
class ImageDecoder {
public:
	ImageDecoder() = delete;
	virtual ~ImageDecoder() = default;
	ImageDecoder(const ImageDecoder &) = delete;
	ImageDecoder &operator=(const ImageDecoder &) = delete;
	static TextureMip Decode(std::span<const uint8_t> encoded);
};
} // namespace VulkanRenderer
