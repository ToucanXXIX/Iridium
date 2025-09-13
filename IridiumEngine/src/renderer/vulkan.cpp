#include "vulkan.hpp"

#include <algorithm>
#include <cstdint>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
#include <vulkan/vulkan_core.h>

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

#include "../log.hpp"

Iridium::Renderer::renderer_error::renderer_error(const std::string& what) : std::runtime_error(what) {};

const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*>& Iridium::getValidationLayers() {
	return validationLayers;
}

const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

const std::vector<const char*>& Iridium::getDeviceExtensions() {
	return deviceExtensions;
}

bool Iridium::queue_family_indices::isComplete() const {
	bool complete = true;
	complete = complete && computeFamily.has_value();
	complete = complete && graphicsFamily.has_value();
	complete = complete && presentFamily.has_value();

	return complete;
}

std::vector<const char*> Iridium::getRequiredExtensions() {
	uint32_t glfwExtensionsCount = 0;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionsCount);
	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionsCount);

	if constexpr(USE_VALIDATION_LAYERS) {
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return extensions;
}

bool Iridium::checkValidationLayers() {
	uint32_t layerCount = 0;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for(const auto& reqLayerRaw : validationLayers) {
		std::string_view reqLayer(reqLayerRaw);
		bool present = false;
		for(const auto& presentLayer : availableLayers) {
			if(reqLayer == presentLayer.layerName) {
				present = true;
				break;
			}
		}
		if(!present)
			return false;
	}

	return true;
}

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		[[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		[[maybe_unused]] void* pUserData
	) {
		switch(messageSeverity) {
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
			ENGINE_LOG_INFO("VK validation layer: {}", pCallbackData->pMessage);
			break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
			ENGINE_LOG_INFO("VK validation layer: {}", pCallbackData->pMessage);
			break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			ENGINE_LOG_WARN("VK validation layer: {}", pCallbackData->pMessage);
			break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			ENGINE_LOG_ERROR("VK validation layer: {}", pCallbackData->pMessage);
			break;
			default:
			break;
		}
		return VK_FALSE;
	}

void Iridium::populateVkDeugUtilsMessengerCreateInfoEXT(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT 
							   | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
						   | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT 
						   | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback;
	createInfo.pUserData = nullptr;
}

Iridium::queue_family_indices Iridium::findQueueFamilies([[maybe_unused]] VkPhysicalDevice device, VkSurfaceKHR surface) {
	queue_family_indices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	size_t iterator = 0;
	for(const auto& queueFamily : queueFamilies) {
		if(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			indices.graphicsFamily = iterator;
		if(queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)
			indices.computeFamily = iterator;
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, iterator, surface, &presentSupport);
		if(presentSupport) 
			indices.presentFamily = iterator;
		if(indices.isComplete()) break;
		iterator++;
	}

	return indices;
}

bool Iridium::checkDeviceExtensionSupport([[maybe_unused]]VkPhysicalDevice device) {
	uint32_t extensionCount = 0;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
	
	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
	for(const auto& extension : availableExtensions) {
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}

bool Iridium::isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface) {
	queue_family_indices indices = findQueueFamilies(device, surface);

	bool extensionsSupported = Iridium::checkDeviceExtensionSupport(device);

	bool swapChainAdequate = false;
	if(extensionsSupported) {
		Iridium::swapchain_support_details swapchainSupport = querySwapchainSupport(device, surface);
		swapChainAdequate = !(
			swapchainSupport.formats.empty() ||
			swapchainSupport.presentModes.empty()
		);
	}

	return indices.isComplete() && extensionsSupported && swapChainAdequate;
}


Iridium::swapchain_support_details Iridium::querySwapchainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) {
	swapchain_support_details details;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

	uint32_t formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
	if(formatCount) {
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
	}

	uint32_t presentModeCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
	if(presentModeCount) {
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
	}

	return details;
}

VkSurfaceFormatKHR Iridium::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) {
	for(const auto& format : formats) {
		if(format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			return format;
	}
	return formats[0];
}

VkPresentModeKHR Iridium::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& modes) {
	for(const auto& mode : modes) {
		if(mode == VK_PRESENT_MODE_MAILBOX_KHR)
			return mode;
	}
	return VK_PRESENT_MODE_FIFO_KHR;
}


VkExtent2D Iridium::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window) {
	if(capabilities.currentExtent.width != UINT32_MAX)
		return capabilities.currentExtent;
	else {
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		VkExtent2D extent{
			.width = static_cast<uint32_t>(width),
			.height =  static_cast<uint32_t>(height)
		};

		extent.width = std::clamp(extent.width, capabilities.maxImageExtent.width, capabilities.maxImageExtent.width);
		extent.height = std::clamp(extent.height, capabilities.maxImageExtent.height, capabilities.maxImageExtent.height);
		return extent;
	}
}