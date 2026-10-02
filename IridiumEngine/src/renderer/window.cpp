#include "window.hpp"

#include <GLFW/glfw3.h>
#include <stdexcept>

#include "../log.hpp"
#include "../utils.hpp"

Iridium::window_manager::window_manager() {
	glfwSetErrorCallback([](int errCode, const char* message) -> void {
		ENGINE_LOG_ERROR("GLFW ERROR ({}): {}", errCode, message);
	});
	glfwInit();
}

Iridium::window_manager::~window_manager() {
	glfwTerminate();
}

void Iridium::window_manager::createWindow(int width, int height, const char* name) {
	if(m_window != nullptr)
		throw std::runtime_error("Cannot create new window when one already exists!");
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	m_window = glfwCreateWindow(width, height, name, nullptr, nullptr);

	glfwSetWindowUserPointer((GLFWwindow*)m_window, (void*)this);

	glfwSetWindowSizeCallback((GLFWwindow*)m_window, [](GLFWwindow* window, int width, int height){
		window_manager& windowManager = *(window_manager*)glfwGetWindowUserPointer(window);
		windowManager.m_windowWidht = width;
		windowManager.m_windowHeight = height;

		if(windowManager.m_callbacks.resizeCallback)
			windowManager.m_callbacks.resizeCallback(width, height);
	});

	glfwSetFramebufferSizeCallback((GLFWwindow*)m_window, [](GLFWwindow* window, int width, int height){
		window_manager& windowManager = *(window_manager*)glfwGetWindowUserPointer(window);
		windowManager.m_framebufferWidth = width;
		windowManager.m_framebufferHeight = height;

		if(windowManager.m_callbacks.framebufferCallback)
			windowManager.m_callbacks.framebufferCallback(width, height);
	});

	//glfwSetWindowRefreshCallback((GLFWwindow*)m_window, [](GLFWwindow*) -> void {});
}



void Iridium::window_manager::setWindowName(const char* name) {
	glfwSetWindowTitle((GLFWwindow*)m_window, name);
}

Iridium::resize_callback Iridium::window_manager::setResizeCallback(resize_callback newCallback) {
	auto old = m_callbacks.resizeCallback;
	m_callbacks.resizeCallback = newCallback;
	return old;
}

Iridium::framebuffer_resize_callback Iridium::window_manager::setFramebufferResizeCallback(framebuffer_resize_callback newCalback) {
	auto old = m_callbacks.framebufferCallback;
	m_callbacks.framebufferCallback = newCalback;
	return old;
}

Iridium::key_callback Iridium::window_manager::setKeyCallback(key_callback newCallback) {
	auto old = m_callbacks.keyCallback;
	m_callbacks.keyCallback = newCallback;
	return old;
}

Iridium::char_callback Iridium::window_manager::setCharCallback(char_callback newCallback) {
	auto old = m_callbacks.charCallback;
	m_callbacks.charCallback = newCallback;
	return old;
}

void Iridium::window_manager::pollEvents() {
	glfwPollEvents();
	GLFWwindow* window = (GLFWwindow*)m_window;
	m_shouldClose = glfwWindowShouldClose(window);

	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	m_framebufferWidth = width;
	m_framebufferHeight = height;
}

std::tuple<uint32_t, uint32_t> Iridium::window_manager::framebufferSize() {
	return {m_framebufferWidth, m_framebufferHeight};
}


bool Iridium::window_manager::windowShouldClose() {
	return m_shouldClose;
}

Iridium::window_ptr Iridium::window_manager::getWindowHandle() {
	return m_window;
}

Iridium::window_manager* Iridium::getWindowManager() {
	return Iridium::getApplicationPointer()->windowManager;
}