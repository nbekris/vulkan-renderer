#include "Rendering/Memory/AllocatedImage.h"
#include "Rendering/Memory/MemoryAllocator.h"
#include "Rendering/Core/VulkanCheck.h"
#include <algorithm>
#include <bit>

namespace VulkanRenderer {
AllocatedImage::AllocatedImage(const MemoryAllocator &allocator) : _allocator(allocator) {
}

AllocatedImage::~AllocatedImage() noexcept {
	if (_view) {
		vkDestroyImageView(_allocator.GetDevice(), _view, nullptr);
	}
	if (_image) {
		vmaDestroyImage(_allocator.GetHandle(), _image, _allocation);
	}
}

void AllocatedImage::Initialize(VkExtent2D extent, VkFormat format, VkImageUsageFlags usage,
								VkImageAspectFlags viewAspect, uint32_t mipLevels) {
	if (_image) {
		throw std::logic_error("image already initialized");
	}
	if (extent.width == 0 || extent.height == 0 || format == VK_FORMAT_UNDEFINED || mipLevels == 0
		|| mipLevels > static_cast<uint32_t>(std::bit_width(std::max(extent.width, extent.height)))) {
		throw std::invalid_argument("image requires a nonzero extent and defined format");
	}
	VkImageCreateInfo image{};
	image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image.imageType = VK_IMAGE_TYPE_2D;
	image.format = format;
	image.extent = {extent.width, extent.height, 1};
	image.mipLevels = mipLevels;
	image.arrayLayers = 1;
	image.samples = VK_SAMPLE_COUNT_1_BIT;
	image.tiling = VK_IMAGE_TILING_OPTIMAL;
	image.usage = usage;
	image.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	VmaAllocationCreateInfo allocation{};
	allocation.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
	allocation.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	CheckVulkan(vmaCreateImage(_allocator.GetHandle(), &image, &allocation, &_image, &_allocation, nullptr),
				"failed to allocate image");
	_extent = extent;
	_mipLevels = mipLevels;
	VkImageViewCreateInfo view{};
	view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	view.image = _image;
	view.viewType = VK_IMAGE_VIEW_TYPE_2D;
	view.format = format;
	view.subresourceRange = {viewAspect, 0, mipLevels, 0, 1};
	CheckVulkan(vkCreateImageView(_allocator.GetDevice(), &view, nullptr, &_view), "failed to create image view");
}
} // namespace VulkanRenderer
