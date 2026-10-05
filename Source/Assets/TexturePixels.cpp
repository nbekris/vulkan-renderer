#include "Assets/TexturePixels.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace VulkanRenderer {
namespace {
float Decode(float value) {
	return value <= 0.04045f ? value / 12.92f : std::pow((value + 0.055f) / 1.055f, 2.4f);
}

float Encode(float value) {
	return value <= 0.0031308f ? value * 12.92f : 1.055f * std::pow(value, 1.0f / 2.4f) - 0.055f;
}

TextureMip Downsample(const TextureMip &source, TextureColorSpace colorSpace) {
	TextureMip result{{std::max(1u, source.extent.width / 2), std::max(1u, source.extent.height / 2)}, {}};
	result.pixels.resize(static_cast<size_t>(result.extent.width) * result.extent.height * 4);
	for (uint32_t y = 0; y < result.extent.height; ++y) {
		for (uint32_t x = 0; x < result.extent.width; ++x) {
			const uint32_t X_START
				= static_cast<uint32_t>(static_cast<uint64_t>(x) * source.extent.width / result.extent.width);
			const uint32_t X_END
				= static_cast<uint32_t>(static_cast<uint64_t>(x + 1) * source.extent.width / result.extent.width);
			const uint32_t Y_START
				= static_cast<uint32_t>(static_cast<uint64_t>(y) * source.extent.height / result.extent.height);
			const uint32_t Y_END
				= static_cast<uint32_t>(static_cast<uint64_t>(y + 1) * source.extent.height / result.extent.height);
			for (size_t channel = 0; channel < 4; ++channel) {
				float sum = 0;
				for (uint32_t sy = Y_START; sy < Y_END; ++sy) {
					for (uint32_t sx = X_START; sx < X_END; ++sx) {
						float value = source.pixels[(static_cast<size_t>(sy) * source.extent.width + sx) * 4 + channel]
									  / 255.0f;
						if (colorSpace == TextureColorSpace::Srgb && channel < 3) {
							value = Decode(value);
						}
						sum += value;
					}
				}
				float average = sum / static_cast<float>((X_END - X_START) * (Y_END - Y_START));
				if (colorSpace == TextureColorSpace::Srgb && channel < 3) {
					average = Encode(average);
				}
				const size_t INDEX = (static_cast<size_t>(y) * result.extent.width + x) * 4 + channel;
				result.pixels[INDEX] = static_cast<uint8_t>(std::lround(std::clamp(average, 0.0f, 1.0f) * 255));
			}
		}
	}
	return result;
}
} // namespace

std::vector<TextureMip> TexturePixels::BuildMipChain(VkExtent2D extent, std::span<const uint8_t> pixels,
													 TextureColorSpace colorSpace) {
	const uint64_t PIXEL_COUNT = static_cast<uint64_t>(extent.width) * extent.height;
	if (extent.width == 0 || extent.height == 0 || PIXEL_COUNT > std::numeric_limits<size_t>::max() / 4
		|| pixels.size() != static_cast<size_t>(PIXEL_COUNT) * 4) {
		throw std::invalid_argument("texture requires exactly width * height * 4 RGBA8 bytes");
	}
	std::vector<TextureMip> mips;
	mips.push_back({extent, {pixels.begin(), pixels.end()}});
	while (mips.back().extent.width > 1 || mips.back().extent.height > 1) {
		mips.push_back(Downsample(mips.back(), colorSpace));
	}
	return mips;
}
} // namespace VulkanRenderer
