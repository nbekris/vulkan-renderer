#include "Rendering/Descriptors/GlobalDescriptors.h"
#include "Rendering/Frames/FrameRing.h"
#include "Rendering/Frames/FrameResources.h"
#include "Rendering/Resources/SceneResources.h"
#include "Rendering/Resources/Material.h"
#include "Rendering/Resources/Texture.h"
#include "Rendering/Core/VulkanContext.h"
#include "Rendering/Core/VulkanCheck.h"
#include "Rendering/RenderData.h"
#include <vector>

namespace VulkanRenderer {

GlobalDescriptors::GlobalDescriptors(const VulkanContext &context, const SceneResources &resources,
									 const FrameRing &frames)
	: _context(context), _resources(resources), _frames(frames) {
}

GlobalDescriptors::~GlobalDescriptors() noexcept {
	if (_pool) {
		vkDestroyDescriptorPool(_context.GetDevice(), _pool, nullptr);
	}
	if (_layout) {
		vkDestroyDescriptorSetLayout(_context.GetDevice(), _layout, nullptr);
	}
}

void GlobalDescriptors::Initialize() {
	if (_layout || !_resources.IsFrozen()) {
		throw std::logic_error("descriptor table requires a fresh instance and frozen scene resources");
	}
	CreateLayout();
	CreatePool();
	WriteDescriptors();
}

void GlobalDescriptors::CreateLayout() {
	const VkDescriptorSetLayoutBinding BINDINGS[]
		= {{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MAX_FRAMES_IN_FLIGHT,
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
		   {1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, MATERIAL_CAPACITY, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
		   {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, TEXTURE_CAPACITY, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}};
	const VkDescriptorBindingFlags FLAGS[]
		= {VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT, VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT,
		   VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT};
	VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlags{};
	bindingFlags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
	bindingFlags.bindingCount = 3;
	bindingFlags.pBindingFlags = FLAGS;
	VkDescriptorSetLayoutCreateInfo layout{};
	layout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layout.pNext = &bindingFlags;
	layout.bindingCount = 3;
	layout.pBindings = BINDINGS;
	CheckVulkan(vkCreateDescriptorSetLayout(_context.GetDevice(), &layout, nullptr, &_layout),
				"failed to create descriptor layout");
}

void GlobalDescriptors::CreatePool() {
	const VkDescriptorPoolSize SIZES[] = {{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MAX_FRAMES_IN_FLIGHT},
										  {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, MATERIAL_CAPACITY},
										  {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, TEXTURE_CAPACITY}};
	VkDescriptorPoolCreateInfo pool{};
	pool.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool.maxSets = 1;
	pool.poolSizeCount = 3;
	pool.pPoolSizes = SIZES;
	CheckVulkan(vkCreateDescriptorPool(_context.GetDevice(), &pool, nullptr, &_pool),
				"failed to create descriptor pool");
	VkDescriptorSetAllocateInfo set{};
	set.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	set.descriptorPool = _pool;
	set.descriptorSetCount = 1;
	set.pSetLayouts = &_layout;
	CheckVulkan(vkAllocateDescriptorSets(_context.GetDevice(), &set, &_set),
				"failed to allocate global descriptor set");
}

void GlobalDescriptors::WriteDescriptors() {
	std::vector<VkDescriptorBufferInfo> uniforms;
	for (uint32_t i = 0; i < _frames.GetCount(); ++i) {
		uniforms.push_back({_frames.GetFrame(i).GetUniformBuffer().GetHandle(), 0, sizeof(FrameUniform)});
	}
	std::vector<VkDescriptorBufferInfo> materials;
	for (uint32_t i = 0; i < _resources.GetMaterialCount(); ++i) {
		materials.push_back({_resources.GetMaterial(i).GetBuffer(), 0, sizeof(MaterialData)});
	}
	std::vector<VkDescriptorImageInfo> textures;
	for (uint32_t i = 0; i < _resources.GetTextureCount(); ++i) {
		textures.push_back(_resources.GetTexture(i).GetDescriptor());
	}
	VkWriteDescriptorSet writes[3]{};
	writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes[0].dstSet = _set;
	writes[0].dstBinding = 0;
	writes[0].descriptorCount = static_cast<uint32_t>(uniforms.size());
	writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	writes[0].pBufferInfo = uniforms.data();
	writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes[1].dstSet = _set;
	writes[1].dstBinding = 1;
	writes[1].descriptorCount = static_cast<uint32_t>(materials.size());
	writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	writes[1].pBufferInfo = materials.data();
	writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes[2].dstSet = _set;
	writes[2].dstBinding = 2;
	writes[2].descriptorCount = static_cast<uint32_t>(textures.size());
	writes[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	writes[2].pImageInfo = textures.data();
	vkUpdateDescriptorSets(_context.GetDevice(), 3, writes, 0, nullptr);
}

bool GlobalDescriptors::IsMaterialBound(uint32_t index) const noexcept {
	return index < _resources.GetMaterialCount();
}

void GlobalDescriptors::Bind(VkCommandBuffer commandBuffer, VkPipelineLayout layout) const {
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &_set, 0, nullptr);
}

} // namespace VulkanRenderer
