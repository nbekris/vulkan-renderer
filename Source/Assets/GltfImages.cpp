#include "Assets/GltfImages.h"
#include "Assets/ImageDecoder.h"
#include "ThirdParty/cgltf/cgltf.h"
#include <fstream>
#include <memory>
#include <cstdlib>
#include <stdexcept>
#include <string>

namespace VulkanRenderer {
namespace {
std::vector<uint8_t> ReadImage(const cgltf_image &image, const std::filesystem::path &directory) {
	if (image.buffer_view) {
		const auto *bytes = cgltf_buffer_view_data(image.buffer_view);
		if (!bytes) {
			throw std::runtime_error("glTF image buffer view is unavailable");
		}
		return {bytes, bytes + image.buffer_view->size};
	}
	if (!image.uri) {
		throw std::runtime_error("glTF image has no URI or buffer view");
	}
	std::string uri = image.uri;
	if (uri.starts_with("data:")) {
		const size_t COMMA = uri.find(',');
		if (COMMA == std::string::npos || uri.substr(0, COMMA).find(";base64") == std::string::npos) {
			throw std::runtime_error("glTF image data URI must contain base64");
		}
		const std::string DATA = uri.substr(COMMA + 1);
		if (DATA.empty() || DATA.size() % 4 != 0) {
			throw std::runtime_error("invalid image base64 length");
		}
		size_t size = DATA.size() / 4 * 3;
		if (DATA.back() == '=') {
			--size;
		}
		if (DATA.size() > 1 && DATA[DATA.size() - 2] == '=') {
			--size;
		}
		void *decoded = nullptr;
		cgltf_options options{};
		if (cgltf_load_buffer_base64(&options, size, DATA.c_str(), &decoded) != cgltf_result_success) {
			throw std::runtime_error("invalid image base64 data");
		}
		std::unique_ptr<void, decltype(&std::free)> owner(decoded, std::free);
		const auto *bytes = static_cast<const uint8_t *>(decoded);
		return {bytes, bytes + size};
	}
	if (uri.find("://") != std::string::npos) {
		throw std::runtime_error("remote glTF image URIs are unsupported");
	}
	cgltf_decode_uri(uri.data());
	uri.resize(std::char_traits<char>::length(uri.c_str()));
	std::ifstream file(directory / std::filesystem::path(std::u8string(uri.begin(), uri.end())),
					   std::ios::binary | std::ios::ate);
	if (!file || file.tellg() <= 0) {
		throw std::runtime_error("cannot read glTF image: " + uri);
	}
	std::vector<uint8_t> bytes(static_cast<size_t>(file.tellg()));
	file.seekg(0);
	if (!file.read(reinterpret_cast<char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()))) {
		throw std::runtime_error("incomplete glTF image: " + uri);
	}
	return bytes;
}

VkSamplerAddressMode Wrap(cgltf_wrap_mode mode) {
	if (mode == cgltf_wrap_mode_clamp_to_edge) {
		return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	}
	if (mode == cgltf_wrap_mode_mirrored_repeat) {
		return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	}
	return VK_SAMPLER_ADDRESS_MODE_REPEAT;
}
} // namespace

ModelTexture GltfImages::Read(const cgltf_texture &texture, const std::filesystem::path &directory) {
	if (!texture.image) {
		throw std::runtime_error("texture requires a PNG/JPEG image fallback");
	}
	ModelTexture result{ImageDecoder::Decode(ReadImage(*texture.image, directory)), {}};
	if (texture.sampler) {
		const auto &SAMPLER = *texture.sampler;
		result.sampling.addressU = Wrap(SAMPLER.wrap_s);
		result.sampling.addressV = Wrap(SAMPLER.wrap_t);
		result.sampling.magFilter
			= SAMPLER.mag_filter == cgltf_filter_type_nearest ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
		const auto MIN = SAMPLER.min_filter;
		result.sampling.minFilter = MIN == cgltf_filter_type_nearest || MIN == cgltf_filter_type_nearest_mipmap_nearest
											|| MIN == cgltf_filter_type_nearest_mipmap_linear
										? VK_FILTER_NEAREST
										: VK_FILTER_LINEAR;
		result.sampling.mipmapMode
			= MIN == cgltf_filter_type_nearest_mipmap_nearest || MIN == cgltf_filter_type_linear_mipmap_nearest
				  ? VK_SAMPLER_MIPMAP_MODE_NEAREST
				  : VK_SAMPLER_MIPMAP_MODE_LINEAR;
		result.sampling.useMipmaps = MIN != cgltf_filter_type_nearest && MIN != cgltf_filter_type_linear;
	}
	return result;
}
} // namespace VulkanRenderer
