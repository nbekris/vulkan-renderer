#include "Rendering/Frames/FrameRing.h"
#include "Rendering/Frames/FrameResources.h"
#include <stdexcept>

namespace VulkanRenderer {

FrameRing::FrameRing(const VulkanContext &context, const MemoryAllocator &allocator)
	: _context(context), _allocator(allocator) {
}

FrameRing::~FrameRing() = default;

FrameResources &FrameRing::GetFrame(uint32_t index) const {
	return *_frames.at(index);
}

void FrameRing::Initialize(uint32_t frameCount, VkExtent2D depthExtent, VkFormat depthFormat, bool profile,
						   bool shadows) {
	if (!_frames.empty()) {
		throw std::logic_error("frame ring already initialized");
	}
	if (frameCount < 2 || frameCount > MAX_FRAMES_IN_FLIGHT) {
		throw std::invalid_argument("frame count must be 2 or 3");
	}
	for (uint32_t i = 0; i < frameCount; ++i) {
		auto frame = std::make_unique<FrameResources>(_context, _allocator);
		frame->Initialize(depthExtent, depthFormat, profile, shadows);
		_frames.push_back(std::move(frame));
	}
}

} // namespace VulkanRenderer
