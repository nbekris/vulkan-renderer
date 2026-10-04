#pragma once
#include <vulkan/vulkan.h>
struct GLFWwindow;

namespace VulkanRenderer {
/** Owns the GLFW session and the native window. */
class Window {
public:
	Window() = default;
	virtual ~Window() noexcept;
	Window(const Window &) = delete;
	Window &operator=(const Window &) = delete;

	GLFWwindow *GetHandle() const noexcept { return _window; }

	bool HasFramebufferResized() const noexcept { return _framebufferResized; }

	bool ShouldClose() const;
	bool IsRenderable() const;
	VkExtent2D GetFramebufferSize() const;
	void Initialize(int width, int height, const char *title);
	void PollEvents() const;
	bool WaitUntilRenderable() const;

	void AcknowledgeFramebufferResize() noexcept { _framebufferResized = false; }

private:
	static void OnFramebufferSizeChanged(GLFWwindow *window, int width, int height);
	bool _glfwInitialized = false;
	bool _framebufferResized = false;
	GLFWwindow *_window = nullptr;
};
} // namespace VulkanRenderer
