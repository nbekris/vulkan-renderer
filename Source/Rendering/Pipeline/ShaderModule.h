#pragma once
#include <vulkan/vulkan.h>
#include <string>

namespace VulkanRenderer {
/** Loads a SPIR-V module and owns its Vulkan handle. */
class ShaderModule {
public:
	explicit ShaderModule(VkDevice device);
	virtual ~ShaderModule() noexcept;
	ShaderModule(const ShaderModule &) = delete;
	ShaderModule &operator=(const ShaderModule &) = delete;

	VkShaderModule GetHandle() const noexcept { return _module; }

	void Initialize(const std::string &path);

private:
	VkDevice _device;
	VkShaderModule _module = VK_NULL_HANDLE;
};
} // namespace VulkanRenderer
