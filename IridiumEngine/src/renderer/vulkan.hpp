#pragma once
#include "GLFW/glfw3.h"
#include <cstdint>
#include <stdexcept>
#include <vector>

#include <vulkan/vulkan_core.h>

namespace Iridium {
	namespace Renderer {
		struct renderer_error : public std::runtime_error {
			renderer_error(const std::string& what);
		};

		class renderer {

		};
	}

	namespace Vulkan {
		//Vulkan lazy loaded functions
		VkResult CreateDebugUtilsMessengerEXT(
			VkInstance instance,
			const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
			const VkAllocationCallbacks* pAllocator,
			VkDebugUtilsMessengerEXT* pMessenger
		);

		void DestroyDebugUtilsMessengerEXT(
			VkInstance instance,
			VkDebugUtilsMessengerEXT debugMessenger,
			const VkAllocationCallbacks* pAllocator
		);

		//Queue families
		struct queue_family_indices {
			enum family_type {
				graphics = 0,
				compute,
				present,
				COUNT
			};

			uint32_t families[family_type::COUNT];
			uint32_t presentFamilies;

			void setFamily(family_type family, uint32_t index);
			bool isFamilyPresent(family_type family) const;

			bool isComplete() const;
			bool isOneIndex() const;
		};

		queue_family_indices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);		

		//Extensions
		std::vector<const char*> getRequiredExtensions();
		const std::vector<const char*>& getDeviceExtensions();

		//Validation layers
		const std::vector<const char*>& getValidationLayers();
		bool checkValidationLayers();

		//Swapchain
		struct swapchain_support_details {
			VkSurfaceCapabilitiesKHR capabilities;
			std::vector<VkSurfaceFormatKHR> formats;
			std::vector<VkPresentModeKHR> presentModes;
		};

		swapchain_support_details querySwapchainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);
		VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
		VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& presentModes);
		VkExtent2D chooseSwapExtent(VkSurfaceCapabilitiesKHR capabilities, GLFWwindow* window);

		//misc
		void populateVkDeugUtilsMessengerCreateInfoEXT(VkDebugUtilsMessengerCreateInfoEXT &createInfo);
	}

	//old
	#if 0
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

	const std::vector<const char*>& getValidationLayers(); //done
	const std::vector<const char*>& getDeviceExtensions(); //done
	
	std::vector<const char*> getRequiredExtensions(); //done
	bool checkValidationLayers(); //done

	void populateVkDeugUtilsMessengerCreateInfoEXT(VkDebugUtilsMessengerCreateInfoEXT& createInfo); //done

	queue_family_indices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);
	bool checkDeviceExtensionSupport(VkPhysicalDevice device);
	bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface);

	swapchain_support_details querySwapchainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& modes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window);
	#endif
}