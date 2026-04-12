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
}

void Iridium::window_manager::setWindowName(const char* name) {
	glfwSetWindowTitle((GLFWwindow*)m_window, name);
}

bool Iridium::window_manager::windowShouldClose() {
	return glfwWindowShouldClose((GLFWwindow*)m_window);
}

Iridium::window_ptr Iridium::window_manager::getWindowHandle() {
	return m_window;
}

Iridium::window_manager* Iridium::getWindowManager() {
	return Iridium::getApplicationPointer()->windowManager;
}