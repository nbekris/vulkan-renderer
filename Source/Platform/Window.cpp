#include "Platform/Window.h"
#include <GLFW/glfw3.h>
#include <stdexcept>

namespace VulkanRenderer {

Window::~Window() noexcept {
	if (_window) {
		glfwDestroyWindow(_window);
	}
	if (_glfwInitialized) {
		glfwTerminate();
	}
}

void Window::Initialize(int width, int height, const char *title) {
	if (_glfwInitialized) {
		throw std::logic_error("window already initialized");
	}
	if (glfwInit() != GLFW_TRUE) {
		throw std::runtime_error("failed to initialize GLFW");
	}
	_glfwInitialized = true;
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	_window = glfwCreateWindow(width, height, title, nullptr, nullptr);
	if (!_window) {
		throw std::runtime_error("failed to create GLFW window");
	}
	glfwSetWindowUserPointer(_window, this);
	glfwSetFramebufferSizeCallback(_window, OnFramebufferSizeChanged);
	glfwSetKeyCallback(_window, OnKey);
}

void Window::OnFramebufferSizeChanged(GLFWwindow *window, int width, int height) {
	static_cast<void>(width);
	static_cast<void>(height);
	auto *owner = static_cast<Window *>(glfwGetWindowUserPointer(window));
	owner->_framebufferResized = true;
}

void Window::OnKey(GLFWwindow *window, int key, int scancode, int action, int modifiers) {
	static_cast<void>(scancode);
	static_cast<void>(modifiers);
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	}
}

bool Window::IsRenderable() const {
	const auto SIZE = GetFramebufferSize();
	return SIZE.width > 0 && SIZE.height > 0 && glfwGetWindowAttrib(_window, GLFW_ICONIFIED) == GLFW_FALSE;
}

bool Window::WaitUntilRenderable() const {
	// Event-driven waiting avoids spinning on zero-sized or minimized windows.
	while (!ShouldClose() && !IsRenderable()) {
		glfwWaitEventsTimeout(0.1);
	}
	return !ShouldClose();
}

bool Window::ShouldClose() const {
	return glfwWindowShouldClose(_window) != GLFW_FALSE;
}

void Window::PollEvents() const {
	glfwPollEvents();
}

VkExtent2D Window::GetFramebufferSize() const {
	int width = 0;
	int height = 0;
	glfwGetFramebufferSize(_window, &width, &height);
	return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
}

} // namespace VulkanRenderer
