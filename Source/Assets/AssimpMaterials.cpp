#include "Assets/AssimpMaterials.h"
#include "Assets/ImageDecoder.h"
#include <assimp/material.h>
#include <assimp/scene.h>
#include <algorithm>
#include <fstream>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <unordered_map>

namespace VulkanRenderer {
namespace {
ModelTexture Texture(const aiScene &scene, const std::filesystem::path &directory, const std::string &name) {
	if (const auto *embedded = scene.GetEmbeddedTexture(name.c_str())) {
		if (!embedded->mHeight) {
			return {ImageDecoder::Decode({reinterpret_cast<const uint8_t *>(embedded->pcData), embedded->mWidth}), {}};
		}
		const size_t COUNT = static_cast<size_t>(embedded->mWidth) * embedded->mHeight;
		if (COUNT > std::numeric_limits<size_t>::max() / 4) {
			throw std::runtime_error("Assimp import: embedded texture is too large");
		}
		ModelTexture result{{{embedded->mWidth, embedded->mHeight}, std::vector<uint8_t>(COUNT * 4)}, {}};
		for (size_t i = 0; i < COUNT; ++i) {
			const auto &PIXEL = embedded->pcData[i];
			std::copy_n(std::array<uint8_t, 4>{PIXEL.r, PIXEL.g, PIXEL.b, PIXEL.a}.begin(), 4,
						result.image.pixels.begin() + i * 4);
		}
		return result;
	}
	auto normalized = name;
	std::replace(normalized.begin(), normalized.end(), '\\', '/');
	const auto FILE = directory / std::filesystem::path(normalized);
	std::ifstream stream(FILE, std::ios::binary);
	if (!stream) {
		throw std::runtime_error("Assimp import: cannot open texture " + FILE.string());
	}
	const std::vector<uint8_t> bytes{std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
	return {ImageDecoder::Decode(bytes), {}};
}
} // namespace

void AssimpMaterials::Read(const aiScene &scene, const std::filesystem::path &directory, ModelData &model) {
	std::unordered_map<std::string, uint32_t> textures;
	for (unsigned i = 0; i < scene.mNumMaterials; ++i) {
		const auto &SOURCE = *scene.mMaterials[i];
		MaterialData material;
		material.textureIndex = NO_MODEL_RESOURCE;
		aiColor4D color(1, 1, 1, 1);
		if (SOURCE.Get(AI_MATKEY_BASE_COLOR, color) != AI_SUCCESS) {
			SOURCE.Get(AI_MATKEY_COLOR_DIFFUSE, color);
		}
		material.tint = {color.r, color.g, color.b, color.a};
		SOURCE.Get(AI_MATKEY_METALLIC_FACTOR, material.metallic);
		SOURCE.Get(AI_MATKEY_ROUGHNESS_FACTOR, material.roughness);
		float opacity = 1;
		SOURCE.Get(AI_MATKEY_OPACITY, opacity);
		if (opacity < 1 || color.a < 1) {
			throw std::runtime_error("Assimp import: transparent materials are unsupported");
		}
		aiString texturePath;
		unsigned uvIndex = 0;
		aiTextureMapMode modes[2]{aiTextureMapMode_Wrap, aiTextureMapMode_Wrap};
		auto type = SOURCE.GetTextureCount(aiTextureType_BASE_COLOR) ? aiTextureType_BASE_COLOR : aiTextureType_DIFFUSE;
		if (SOURCE.GetTexture(type, 0, &texturePath, nullptr, &uvIndex, nullptr, nullptr, modes) == AI_SUCCESS) {
			if (uvIndex != 0) {
				throw std::runtime_error("Assimp import: only texture UV set zero is supported");
			}
			const std::string NAME = texturePath.C_Str();
			// Sampler state is part of the texture identity in the current descriptor table.
			const auto KEY = NAME + ":" + std::to_string(modes[0]) + ":" + std::to_string(modes[1]);
			if (!textures.contains(KEY)) {
				auto texture = Texture(scene, directory, NAME);
				const auto WRAP = [](aiTextureMapMode mode) {
					if (mode == aiTextureMapMode_Decal) {
						throw std::runtime_error("Assimp import: decal texture addressing is unsupported");
					}
					return mode == aiTextureMapMode_Clamp	 ? VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
						   : mode == aiTextureMapMode_Mirror ? VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT
															 : VK_SAMPLER_ADDRESS_MODE_REPEAT;
				};
				texture.sampling.addressU = WRAP(modes[0]);
				texture.sampling.addressV = WRAP(modes[1]);
				textures[KEY] = static_cast<uint32_t>(model.textures.size());
				model.textures.push_back(std::move(texture));
			}
			material.textureIndex = textures.at(KEY);
		}
		model.materials.push_back(material);
	}
}
} // namespace VulkanRenderer
