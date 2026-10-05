#include "Rendering/Pipeline/ShaderModule.h"
#include "Rendering/Core/VulkanCheck.h"
#include <fstream>
#include <vector>

namespace VulkanRenderer {

ShaderModule::ShaderModule(VkDevice device) : _device(device) {
}

ShaderModule::~ShaderModule() noexcept {
	if (_module) {
		vkDestroyShaderModule(_device, _module, nullptr);
	}
}

void ShaderModule::Initialize(const std::string &path) {
	if (_module) {
		throw std::logic_error("shader module already initialized");
	}
	std::ifstream file(path, std::ios::ate | std::ios::binary);
	if (!file) {
		throw std::runtime_error("failed to open shader: " + path);
	}
	const auto SIZE = file.tellg();
	if (SIZE <= 0 || SIZE % sizeof(uint32_t) != 0) {
		throw std::runtime_error("invalid SPIR-V size: " + path);
	}
	// uint32_t storage guarantees the alignment required by VkShaderModuleCreateInfo.
	std::vector<uint32_t> code(static_cast<size_t>(SIZE) / sizeof(uint32_t));
	file.seekg(0);
	if (!file.read(reinterpret_cast<char *>(code.data()), static_cast<std::streamsize>(SIZE))) {
		throw std::runtime_error("failed to read shader: " + path);
	}
	VkShaderModuleCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	info.codeSize = static_cast<size_t>(SIZE);
	info.pCode = code.data();
	CheckVulkan(vkCreateShaderModule(_device, &info, nullptr, &_module), "failed to create shader module");
}

} // namespace VulkanRenderer
