#include "vulkan.hpp"

#include <algorithm>
#include <cstdint>
#include <ratio>
#include <stdexcept>
#include <string>
#include <vector>
#include <chrono>

#include <vulkan/vulkan_core.h>

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

#include "../log.hpp"

namespace IrV = Iridium::Vulkan;
namespace IrR = Iridium::Renderer;

IrR::renderer_error::renderer_error(const std::string& what) : std::runtime_error(what) {};

//NEW

//Constants
const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation",
	//"VK_LAYER_MESA_overlay"
};

const std::vector<const char*> instanceExtensions = {
	"VK_EXT_surface_maintenance1",
	"VK_KHR_get_surface_capabilities2"
};

const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	"VK_EXT_swapchain_maintenance1",
	"VK_EXT_extended_dynamic_state3",
	VK_EXT_SHADER_OBJECT_EXTENSION_NAME
};

//Lazy loaded funcs
VkResult IrV::CreateDebugUtilsMessengerEXT(
	VkInstance instance,
	const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
	const VkAllocationCallbacks *pAllocator,
	VkDebugUtilsMessengerEXT *pMessenger) {
	auto loadedFunc = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if(loadedFunc) {
		return loadedFunc(instance, pCreateInfo, pAllocator, pMessenger);
	} else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}

}

void IrV::DestroyDebugUtilsMessengerEXT(
	VkInstance instance,
	VkDebugUtilsMessengerEXT debugMessenger, 
	const VkAllocationCallbacks *pAllocator) {
	auto loadedFunc = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
		instance,
		"vkDestroyDebugUtilsMessengerEXT");
	if(loadedFunc)
		loadedFunc(instance, debugMessenger, pAllocator);
}

void IrV::CmdSetPolygonModeEXT(
	VkInstance instance,
	VkCommandBuffer commandBuffer,
	VkPolygonMode polygonMode) {
	static auto loadedFunc = (PFN_vkCmdSetPolygonModeEXT)vkGetInstanceProcAddr(
		instance,
		"vkCmdSetPolygonModeEXT"
	);
	if(loadedFunc)
		loadedFunc(commandBuffer, polygonMode);
}

//Queue families

void IrV::queue_family_indices::setFamily(IrV::queue_family_indices::family_type family, uint32_t index) {
	families[family] = index;
	presentFamilies = presentFamilies | (1 << family);
}

bool IrV::queue_family_indices::isFamilyPresent(IrV::queue_family_indices::family_type family) const {
	return ((presentFamilies & (1 << family)) != 0);
}

bool IrV::queue_family_indices::isComplete() const {
	bool ret = true;
	ret = ret && isFamilyPresent(graphics);
	ret = ret && isFamilyPresent(compute);
	ret = ret && isFamilyPresent(present);
	ret = ret && isFamilyPresent(transfer);
	return ret;
}

bool IrV::queue_family_indices::isOneIndex() const {
	return (families[graphics] == families[compute])  && (families[graphics] == families[present]);
}

IrV::queue_family_indices IrV::findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) {
	static auto clock = std::chrono::steady_clock{};
	auto start = clock.now();

	bool cached = true;
	static IrV::queue_family_indices indices = [&device, &surface, &cached]() -> IrV::queue_family_indices {
		cached = false;
		
		IrV::queue_family_indices result{};
		
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
		ENGINE_LOG_INFO("found {} queue families", queueFamilies.size());
		size_t iterator = 0;
		for(const auto& queueFamily : queueFamilies) {
			if(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				result.setFamily(queue_family_indices::graphics, iterator);
			}
			if(queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) {
				result.setFamily(queue_family_indices::compute, iterator);
			}
			if(queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) {
				result.setFamily(queue_family_indices::transfer, iterator);
			}
			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, iterator, surface, &presentSupport);
			if(presentSupport) {
				result.setFamily(queue_family_indices::present, iterator);
			}

			if(result.isComplete())
				break;
			iterator++;
		}
		return result;
	}();

	auto end = clock.now();
	[[maybe_unused]] auto time = std::chrono::duration<double, std::milli>(end - start);
	/*
	ENGINE_LOG_INFO("findQueueFamilies() took {:.3} (cached? {})", time, cached);
	if(indices.isComplete()) {
		ENGINE_LOG_INFO("Found complete queue family with index {}", indices.families[queue_family_indices::graphics]);
	} else {
		ENGINE_LOG_INFO("Found incomplete queue family");
	}
	ENGINE_LOG_INFO_NP("graphics: {} ({})", indices.isFamilyPresent(queue_family_indices::graphics), indices.families[queue_family_indices::graphics]);
	ENGINE_LOG_INFO_NP("compute:  {} ({})", indices.isFamilyPresent(queue_family_indices::compute),  indices.families[queue_family_indices::compute]);
	ENGINE_LOG_INFO_NP("present:  {} ({})", indices.isFamilyPresent(queue_family_indices::present),  indices.families[queue_family_indices::present]);
	ENGINE_LOG_INFO_NP("transfer: {} ({})", indices.isFamilyPresent(queue_family_indices::transfer), indices.families[queue_family_indices::transfer]);
	*/
	return indices;
}

//Extensions

std::vector<const char*> IrV::getRequiredExtensions() {
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
	if constexpr(USE_VALIDATION_LAYERS) {
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}
	for(const auto extension : instanceExtensions) {
		extensions.push_back(extension);
	}
	return extensions;
}

const std::vector<const char*>& IrV::getDeviceExtensions() {
	return deviceExtensions;
}

//Validation layers

const std::vector<const char*>& IrV::getValidationLayers() {
	return validationLayers;
}

bool IrV::checkValidationLayers() {
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

// Swapchain

IrV::swapchain_support_details IrV::querySwapchainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) {
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

VkSurfaceFormatKHR IrV::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) {
	for(const auto& format : formats) {
		if(format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			return format;
	}
	return formats[0];
}

VkPresentModeKHR IrV::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& presentModes) {
	for(const auto& mode : presentModes) {
		if(mode == VK_PRESENT_MODE_MAILBOX_KHR)
			return mode;
		}
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D IrV::chooseSwapExtent(VkSurfaceCapabilitiesKHR capabilities, GLFWwindow* window) {
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

// device

//TODO: make this actualy check device suitability
bool IrV::isDeviceSuitable([[maybe_unused]] VkPhysicalDevice device) {
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);
	
	uint32_t presentDeviceExtensionCount = 0;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &presentDeviceExtensionCount, nullptr);
	std::vector<VkExtensionProperties> presentExtensions(presentDeviceExtensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &presentDeviceExtensionCount, presentExtensions.data());
	//ENGINE_LOG_INFO("Found device extensions:");
	//for(const auto& extension : presentExtensions) {
	//	ENGINE_LOG_INFO_NP("-> {}", extension.extensionName);
	//}

	uint32_t presentLayersCount = 0;
	vkEnumerateDeviceLayerProperties(device, &presentLayersCount, nullptr);
	std::vector<VkLayerProperties> presentLayers(presentLayersCount);
	vkEnumerateDeviceLayerProperties(device, &presentLayersCount, presentLayers.data());
	//ENGINE_LOG_INFO("Found device layers:");
	//for(const auto& layer : presentLayers) {
	//	ENGINE_LOG_INFO_NP("-> {}", layer.layerName);
	//}
	return true;
}

// shader

VkShaderModule IrV::createShaderModule(const std::vector<uint32_t>& compiledShader, VkDevice device) {
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = compiledShader.size() * sizeof(uint32_t);
	createInfo.pCode = compiledShader.data();

	VkShaderModule shaderModule;
	if(vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
		throw IrR::renderer_error("Failed to create shader module");
	return shaderModule;
}

// misc

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
		ENGINE_LOG_FATAL("VK validation layer: {}", pCallbackData->pMessage);
		break;
	}
	return VK_FALSE;
}

void IrV::populateVkDeugUtilsMessengerCreateInfoEXT(VkDebugUtilsMessengerCreateInfoEXT &createInfo) {createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT 
							   | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
						   | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT 
						   | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback;
	createInfo.pUserData = nullptr;
}

//OLD
#if 0
const std::vector<const char*>& Iridium::getValidationLayers() {
	return validationLayers;
}

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
#endif