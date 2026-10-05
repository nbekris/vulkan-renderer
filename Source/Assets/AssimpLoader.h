#pragma once
#include "Assets/ModelData.h"
#include <filesystem>

namespace VulkanRenderer {
class AssimpLoader {
public:
	AssimpLoader() = delete;
	virtual ~AssimpLoader() = default;
	AssimpLoader(const AssimpLoader &) = delete;
	AssimpLoader &operator=(const AssimpLoader &) = delete;
	static ModelData Read(const std::filesystem::path &path);
};
} // namespace VulkanRenderer
