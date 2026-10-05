#pragma once

namespace VulkanRenderer {
class Camera;
class Window;

/** Adapts GLFW input to camera movement without coupling the camera to a window. */
class CameraController {
public:
	CameraController() = default;
	virtual ~CameraController() = default;
	CameraController(const CameraController &) = delete;
	CameraController &operator=(const CameraController &) = delete;
	void Update(const Window &window, Camera &camera);

private:
	double _lastTime = 0;
	double _lastX = 0;
	double _lastY = 0;
	bool _initialized = false;
	bool _looking = false;
};
} // namespace VulkanRenderer
