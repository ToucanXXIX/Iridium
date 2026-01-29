#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

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

			bool m_framebufferResized = false;

			void initVulkan();
			void initWindow();

			void createInstance();
		};
	}
}