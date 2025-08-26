#pragma once
#include "GLFW/glfw3.h"
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <vector>

#include <vulkan/vulkan_core.h>

namespace Iridium {
	namespace Renderer {
		struct renderer_error : public std::runtime_error {
			renderer_error(const std::string& what);
		};

	}
	struct queue_family_indices {
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> computeFamily;
		std::optional<uint32_t> presentFamily;
		
		bool isComplete() const;
	};

	struct swapchain_support_details {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	const std::vector<const char*>& getValidationLayers();
	const std::vector<const char*>& getDeviceExtensions();
	
	
	std::vector<const char*> getRequiredExtensions();
	bool checkValidationLayers();

	void populateVkDeugUtilsMessengerCreateInfoEXT(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

	queue_family_indices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);
	bool checkDeviceExtensionSupport(VkPhysicalDevice device);
	bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface);

	swapchain_support_details querySwapchainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& modes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window);
}