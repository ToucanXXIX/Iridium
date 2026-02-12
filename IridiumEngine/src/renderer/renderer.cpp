#include "renderer.hpp"

#include "vulkan.hpp"
#include "../log.hpp"

#include <cstdint>
#include <format>
#include <set>
#include <ranges>

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

// vulkan setup
void Iridium::Renderer::renderer::initVulkan() {
	createInstance();
	setupDebugMessenger();
	createSurface();
	pickPhysicalDevice();
	createLogicalDevice();
	createSwapchain();
	createImageViews();
	createRenderPass();
	createGraphicsPipeline();
	createFramebuffers();
	createCommandPool();
	createCommandBuffers();
	createSyncObjects();
}

void Iridium::Renderer::renderer::createInstance() {
	if(USE_VALIDATION_LAYERS && Vulkan::checkValidationLayers()) {
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

	auto& validationLayers = Iridium::Vulkan::getValidationLayers();
	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
	if constexpr(USE_VALIDATION_LAYERS) {
		Iridium::Vulkan::populateVkDeugUtilsMessengerCreateInfoEXT(debugCreateInfo);
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
		createInfo.pNext = &debugCreateInfo;
	}

	VkResult createResult = vkCreateInstance(&createInfo, nullptr, &m_instance);
	if(createResult != VK_SUCCESS) {
		throw Iridium::Renderer::renderer_error(std::format("Failed to create vk instance with error code: {}", (size_t)createResult));
	}
}

void Iridium::Renderer::renderer::setupDebugMessenger() {
	if constexpr(!USE_VALIDATION_LAYERS) return;

	VkDebugUtilsMessengerCreateInfoEXT createInfo{};
	Iridium::Vulkan::populateVkDeugUtilsMessengerCreateInfoEXT(createInfo);

	VkResult result = Vulkan::CreateDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, &m_debugMessenger);
	if(result != VK_SUCCESS) {
		throw std::runtime_error("Failed to set up debug messenger.");
	}
}

void Iridium::Renderer::renderer::createSurface() {
	if(glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface) != VK_SUCCESS) {
		throw Iridium::Renderer::renderer_error("failed to create window surface");
	}
}

void Iridium::Renderer::renderer::pickPhysicalDevice() {
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);
	if(deviceCount == 0)
		throw Iridium::Renderer::renderer_error("Failed to find suitable gpu.");

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

	for(const auto& device : devices) {
		if(Vulkan::isDeviceSuitable(device)) {
			m_physicalDevice = device;
			break;
		}
	}

	if(m_physicalDevice == VK_NULL_HANDLE)
		throw Iridium::Renderer::renderer_error("Failed to find suitable gpu.");

	VkPhysicalDeviceProperties properties;
	vkGetPhysicalDeviceProperties(m_physicalDevice, &properties);

	ENGINE_LOG_INFO("Device name: {}", properties.deviceName);
}

void Iridium::Renderer::renderer::createLogicalDevice() {
	using enum Iridium::Vulkan::queue_family_indices::family_type;

	Iridium::Vulkan::queue_family_indices indices = Iridium::Vulkan::findQueueFamilies(m_physicalDevice, m_surface);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = {
		indices.families[graphics],
		indices.families[compute],
		indices.families[present]
	};

	float queuePriority = 0.0f;
	for(uint32_t queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures{};
	auto& deviceExtensions = Iridium::Vulkan::getDeviceExtensions();
	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.queueCreateInfoCount = queueCreateInfos.size();
	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();
	createInfo.enabledExtensionCount = deviceExtensions.size();
	if constexpr(USE_VALIDATION_LAYERS) {
		auto& validationLayers = Iridium::Vulkan::getValidationLayers();
		createInfo.enabledLayerCount = validationLayers.size();
		createInfo.ppEnabledLayerNames = validationLayers.data();
	} else {
		createInfo.enabledLayerCount = 0;
	}

	if(vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device) != VK_SUCCESS)
		throw Iridium::Renderer::renderer_error("Failed to create logical device.");

	vkGetDeviceQueue(m_device, indices.families[graphics], 0, &m_graphicsQueue);
	vkGetDeviceQueue(m_device, indices.families[present], 0, &m_graphicsQueue);
	vkGetDeviceQueue(m_device, indices.families[compute], 0, &m_computeQueue);
}

void Iridium::Renderer::renderer::createSwapchain() {
	Iridium::Vulkan::swapchain_support_details swapchainSupport = Iridium::Vulkan::querySwapchainSupport(m_physicalDevice, m_surface);
	VkSurfaceFormatKHR surfaceFormat = Iridium::Vulkan::chooseSwapSurfaceFormat(swapchainSupport.formats);
	VkPresentModeKHR presentMode = Iridium::Vulkan::chooseSwapPresentMode(swapchainSupport.presentModes);
	VkExtent2D extent = Iridium::Vulkan::chooseSwapExtent(swapchainSupport.capabilities, m_window);
	uint32_t imageCount = swapchainSupport.capabilities.minImageCount + 1;
	if(swapchainSupport.capabilities.maxImageCount > 0 && imageCount > swapchainSupport.capabilities.maxImageCount) {
		imageCount = swapchainSupport.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = m_surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	using enum Iridium::Vulkan::queue_family_indices::family_type;
	Iridium::Vulkan::queue_family_indices indices = Iridium::Vulkan::findQueueFamilies(m_physicalDevice, m_surface);
	uint32_t queueFamilyIndices[] = {
		indices.families[graphics],
		indices.families[compute],
		indices.families[present]
	};
	if(!indices.isOneIndex()) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 3;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	} else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = nullptr;
	}

	createInfo.preTransform = swapchainSupport.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;
	if(vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapchain) != VK_SUCCESS)
		throw Iridium::Renderer::renderer_error("failed to create swapchain");

	vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, nullptr);
	m_swapchainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, m_swapchainImages.data());
	
	m_swapchainImageFormat = surfaceFormat.format;
	m_swapchainExtent = extent;
}

void Iridium::Renderer::renderer::createImageViews() {
	m_swapchainImageViews.resize(m_swapchainImages.size());
	for (auto [index, image] : std::views::enumerate(m_swapchainImages)) {
		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = image;
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = m_swapchainImageFormat;
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.layerCount = 1;
		if(vkCreateImageView(m_device, &createInfo, nullptr, &m_swapchainImageViews[index]) != VK_SUCCESS) {
			throw Iridium::Renderer::renderer_error("Failed to create image views.");
		}
	}
}

void Iridium::Renderer::renderer::createRenderPass() {
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = m_swapchainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0; //references the layout(location = 0) out vec4 outColor !!!
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;
	if(vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS)
		throw Iridium::Renderer::renderer_error("failed to create render pass");
}

void Iridium::Renderer::renderer::createGraphicsPipeline() {

}