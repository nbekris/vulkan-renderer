#pragma once
#include <string>

namespace VulkanRenderer {
/** Composition root: selects components and controls the event loop. */
class Application {
public:
	Application() = default;
	virtual ~Application() = default;
	Application(const Application &) = delete;
	Application &operator=(const Application &) = delete;
	void Run(int frameLimit = 0, unsigned framesInFlight = 2, const std::string &modelPath = "", bool benchmark = false,
			 bool culling = true, bool autoFrame = true);
};
} // namespace VulkanRenderer
