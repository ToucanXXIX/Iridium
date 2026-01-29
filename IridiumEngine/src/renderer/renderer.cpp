#include "renderer.hpp"

#include "vulkan.hpp"
#include <vulkan/vulkan_core.h>

namespace IrV = Iridium::Vulkan;

void Iridium::Renderer::renderer::initWindow() {
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	m_window = glfwCreateWindow(800, 600, m_info.name, nullptr, nullptr);
	glfwSetWindowUserPointer(m_window, this);
	glfwSetFramebufferSizeCallback(m_window, [](GLFWwindow* window, int, int) -> void {
		renderer& renderer = *reinterpret_cast<class renderer*>(glfwGetWindowUserPointer(window));
		renderer.m_framebufferResized = true;
	});
}

void Iridium::Renderer::renderer::initVulkan() {
	createInstance();
}

void Iridium::Renderer::renderer::createInstance() {
	if(USE_VALIDATION_LAYERS && Iridium::checkValidationLayers()) {
		throw Iridium::Renderer::renderer_error("validation layer unavailable.");
	}

	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pNext = nullptr;
	appInfo.pApplicationName = m_info.name;
	appInfo.applicationVersion = VK_MAKE_VERSION(m_info.version.major, m_info.version.minor, m_info.version.patch);
	appInfo.pEngineName = "Iridium";
	appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
	appInfo.apiVersion = VK_API_VERSION_1_1;

	auto extensions = IrV::getRequiredExtensions();
	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = extensions.size();
	createInfo.ppEnabledExtensionNames = extensions.data();
}