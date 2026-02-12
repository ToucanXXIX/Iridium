#pragma once

#include <vector>

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
			VkDevice m_device;
			VkQueue m_graphicsQueue;
			VkQueue m_computeQueue;
			VkQueue m_presentsQueue;
			VkSwapchainKHR m_swapchain;
			std::vector<VkImage> m_swapchainImages;
			VkFormat m_swapchainImageFormat;
			VkExtent2D m_swapchainExtent;
			std::vector<VkImageView> m_swapchainImageViews;
			VkRenderPass m_renderPass;

			bool m_framebufferResized = false;

			void initWindow();
			void initVulkan();

			void createInstance();
			void setupDebugMessenger();
			void createSurface();
			void pickPhysicalDevice();
			void createLogicalDevice();
			void createSwapchain();
			void createImageViews();
			void createRenderPass();
			void createGraphicsPipeline();
		};
	}
}