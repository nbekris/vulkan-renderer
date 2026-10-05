#pragma once
#include <vulkan/vulkan.h>
#include <cstdint>
#include <span>
#include <vector>

namespace VulkanRenderer {
enum class TextureColorSpace { Srgb, Linear };

struct TextureMip {
	VkExtent2D extent;
	std::vector<uint8_t> pixels;
};

/** RGBA8 mip generation; sRGB RGB channels are averaged in linear space, alpha is always linear. */
class TexturePixels {
public:
	TexturePixels() = delete;
	virtual ~TexturePixels() = default;
	TexturePixels(const TexturePixels &) = delete;
	TexturePixels &operator=(const TexturePixels &) = delete;
	static std::vector<TextureMip> BuildMipChain(VkExtent2D extent, std::span<const uint8_t> pixels,
												 TextureColorSpace colorSpace);
};
} // namespace VulkanRenderer
