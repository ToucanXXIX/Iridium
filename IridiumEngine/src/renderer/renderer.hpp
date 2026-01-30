#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>

#include "../appinfo.hpp"

namespace Iridium {
	namespace Renderer {
		using window_ptr = GLFWwindow*;

		class renderer {
		public:
			renderer(const appinfo& info);
		private:
			const appinfo& m_info;
			window_ptr m_window;

			//Vulkan
			VkInstance m_instance;
			VkDebugUtilsMessengerEXT m_debugMessenger;
			VkSurfaceKHR m_surface;
			VkPhysicalDevice m_physicalDevice;

			bool m_framebufferResized = false;

			void initWindow();
			void initVulkan();

			void createInstance();
			void setupDebugMessenger();
			void createSurface();
			void pickPhysicalDevice();
		};
	}
}