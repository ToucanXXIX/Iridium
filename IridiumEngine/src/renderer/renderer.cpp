#include "renderer.hpp"

#include "GLFW/glfw3.h"

#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/fwd.hpp"
#include "glm/trigonometric.hpp"

#include "vertex.hpp"
#include "vulkan.hpp"
#include "../log.hpp"
#include "../utils.hpp"
#include "window.hpp"
#include "shader.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <format>
#include <set>
#include <ranges>

#include <vulkan/vulkan_core.h>

namespace IrV = Iridium::Vulkan;

Iridium::Renderer::renderer::renderer(appinfo& info)
	:m_info(info) {
	ENGINE_LOG_INFO("Initializing engine renderer.");

	m_rendererStart = std::chrono::steady_clock::now();

	initVulkan();
}

// vulkan setup
void Iridium::Renderer::renderer::initVulkan() {
	createInstance();
	setupDebugMessenger();
	createSurface();
	setWindowCallbacks();
	pickPhysicalDevice();
	createLogicalDevice();
	createSwapchain();
	createImageViews();
	createRenderPass();
	createDescriptorSetLayout();
	createGraphicsPipeline();
	createFramebuffers();
	createCommandPool();
	createVertexBuffer();
	createIndexBuffer();
	createUniformBuffers();
	createDescriptorPool();
	createDescriptorSets();
	createCommandBuffers();
	createSyncObjects();
}

void Iridium::Renderer::renderer::cleanupVulkan() {
	destroySyncObjects();
	vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
	cleanupVertexBuffer();
	cleanupIndexBuffer();
	cleanupUniformBuffers();
	vkDestroyCommandPool(m_device, m_commandPool, nullptr);
	cleanupSwapchain();
	vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayout, nullptr);
	vkDestroyPipeline(m_device, m_graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
	vkDestroyRenderPass(m_device, m_renderPass, nullptr);
	vkDestroyDevice(m_device, nullptr);
	if constexpr(USE_VALIDATION_LAYERS)
		IrV::DestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
	vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
	vkDestroyInstance(m_instance, nullptr);
}

void Iridium::Renderer::renderer::createInstance() {
	if(USE_VALIDATION_LAYERS && !Vulkan::checkValidationLayers()) {
		throw Iridium::Renderer::renderer_error("Validation layer unavailable.");
	}

	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pNext = nullptr;
	appInfo.pApplicationName = m_info.name;
	appInfo.applicationVersion = VK_MAKE_VERSION(m_info.version.major, m_info.version.minor, m_info.version.patch);
	appInfo.pEngineName = "Iridium";
	appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
	appInfo.apiVersion = VK_API_VERSION_1_3;

	std::vector<const char*> layers;
	layers.push_back("VK_LAYER_KHRONOS_shader_object");
	if constexpr(USE_VALIDATION_LAYERS) {
		ENGINE_LOG_INFO("Using validation layers");
		for(const auto layer : Iridium::Vulkan::getValidationLayers())
			layers.push_back(layer);
	}
	auto extensions = IrV::getRequiredExtensions();
	for(const auto& ext : extensions) {
		ENGINE_LOG_WARN("Enabled instance extension: {}", ext);
	}
	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();
	createInfo.ppEnabledLayerNames = layers.data();
	createInfo.enabledLayerCount = layers.size();

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
	GLFWwindow* window = (GLFWwindow*)getWindowManager()->getWindowHandle();
	if(glfwCreateWindowSurface(m_instance, window, nullptr, &m_surface) != VK_SUCCESS) {
		throw Iridium::Renderer::renderer_error("failed to create window surface");
	}
}

void Iridium::Renderer::renderer::setWindowCallbacks() {
	GLFWwindow* window = (GLFWwindow*)getWindowManager()->getWindowHandle();
	glfwSetFramebufferSizeCallback(window, [](GLFWwindow*, int, int) -> void {
		renderer* renderer = getApplicationPointer()->renderer;
		renderer->m_framebufferResized = true;
	});
}

void Iridium::Renderer::renderer::pickPhysicalDevice() {
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);
	if(deviceCount == 0)
		throw Iridium::Renderer::renderer_error("Failed to find suitable gpu.");

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

	//TODO(): rank devices, then choose the best device instead of picking the first suitable one 
	for(const auto& device : devices) {
		if(Vulkan::isDeviceSuitable(device)) {
			m_physicalDevice = device;
			break;
		}
	}

	if(m_physicalDevice == VK_NULL_HANDLE)
		throw Iridium::Renderer::renderer_error("Failed to find suitable gpu.");

	VkPhysicalDeviceProperties properties{};
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
		indices.families[present],
		indices.families[transfer]
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
	deviceFeatures.fillModeNonSolid = VK_TRUE;

	auto& deviceExtensions = Iridium::Vulkan::getDeviceExtensions();

	//TODO(): move this to separate function to make the chain automatically.
	VkPhysicalDeviceExtendedDynamicState3FeaturesEXT extendedDynamicState3{};
	extendedDynamicState3.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_3_FEATURES_EXT;
	extendedDynamicState3.extendedDynamicState3PolygonMode = VK_TRUE; //enable wireframe withouth re-creating pipeline

	VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT swapchainMaintenance1{};
	swapchainMaintenance1.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SWAPCHAIN_MAINTENANCE_1_FEATURES_EXT;
	swapchainMaintenance1.swapchainMaintenance1 = VK_TRUE; // fencing on images
	swapchainMaintenance1.pNext = &extendedDynamicState3;

	VkPhysicalDeviceShaderObjectFeaturesEXT shaderObject{};
	shaderObject.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_OBJECT_FEATURES_EXT;
	shaderObject.shaderObject = VK_TRUE;
	shaderObject.pNext = &swapchainMaintenance1;

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.queueCreateInfoCount = queueCreateInfos.size();
	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();
	createInfo.enabledExtensionCount = deviceExtensions.size();
	createInfo.enabledLayerCount = 0; //DONT USE, DEPRECATED
	createInfo.ppEnabledLayerNames = nullptr; //DONT USE, DEPRECATED
	createInfo.pNext = &shaderObject;

	if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device) != VK_SUCCESS)
		throw Iridium::Renderer::renderer_error("Failed to create logical device.");

	vkGetDeviceQueue(m_device, indices.families[graphics], 0, &m_graphicsQueue);
	vkGetDeviceQueue(m_device, indices.families[present], 0, &m_presentQueue);
	vkGetDeviceQueue(m_device, indices.families[compute], 0, &m_computeQueue);
	vkGetDeviceQueue(m_device, indices.families[transfer], 0, &m_transferQueue);
}

void Iridium::Renderer::renderer::createSwapchain() {
	GLFWwindow* window = (GLFWwindow*)getApplicationPointer()->windowManager->getWindowHandle();

	Iridium::Vulkan::swapchain_support_details swapchainSupport = Iridium::Vulkan::querySwapchainSupport(m_physicalDevice, m_surface);
	VkSurfaceFormatKHR surfaceFormat = Iridium::Vulkan::chooseSwapSurfaceFormat(swapchainSupport.formats);
	VkPresentModeKHR presentMode = Iridium::Vulkan::chooseSwapPresentMode(swapchainSupport.presentModes);
	VkExtent2D extent = Iridium::Vulkan::chooseSwapExtent(swapchainSupport.capabilities, window);
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
		indices.families[present],
		indices.families[transfer]
	};
	if(!indices.isOneIndex()) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 4;
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
	createInfo.oldSwapchain = m_swapchain;
	if(vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapchain) != VK_SUCCESS)
		throw Iridium::Renderer::renderer_error("failed to create swapchain");

	vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, nullptr);
	m_swapchainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, m_swapchainImages.data());
	
	m_swapchainImageFormat = surfaceFormat.format;
	m_swapchainExtent = extent;
}

void Iridium::Renderer::renderer::recreateSwapchain() {
	GLFWwindow* window = (GLFWwindow*)getApplicationPointer()->windowManager->getWindowHandle();

	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	while(width == 0 || height == 0) {
		glfwGetFramebufferSize(window, &width, &height);
		glfwWaitEvents();
	}
	vkDeviceWaitIdle(m_device);

	cleanupSwapchain();
	
	createSwapchain();
	createImageViews();
	createFramebuffers();
}

void Iridium::Renderer::renderer::cleanupSwapchain() {
	for(auto& framebuffer : m_swapchainFrameBuffers) {
		vkDestroyFramebuffer(m_device, framebuffer, nullptr);
	}
	for(auto imageView : m_swapchainImageViews) {
		vkDestroyImageView(m_device, imageView, nullptr);
	}
	vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
	m_swapchain = VK_NULL_HANDLE;
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

void Iridium::Renderer::renderer::createDescriptorSetLayout() {
	VkDescriptorSetLayoutBinding layoutBinding{};
	layoutBinding.binding = 0;
	layoutBinding.descriptorCount = 1;
	layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	layoutBinding.pImmutableSamplers = nullptr;
	layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	createInfo.bindingCount = 1;
	createInfo.pBindings = &layoutBinding;

	if(vkCreateDescriptorSetLayout(m_device, &createInfo, nullptr, &m_descriptorSetLayout) != VK_SUCCESS) {
		throw Renderer::renderer_error("Failed to create descriptor set layout.");
	}
}

void Iridium::Renderer::renderer::createGraphicsPipeline() {
	auto& shaderCompiler = *getApplicationPointer()->shaderCompiler;

	std::string vertShader =
	R"(
	#version 450

	layout(location = 0) in vec3 inPosition;
	layout(location = 1) in vec3 inColor;
	layout(location = 2) in vec2 inUV;

	layout(location = 0) out vec3 fragColor;
	layout(location = 1) out vec2 UVcoord;

	layout(binding = 0) uniform UniformBufferObject {
		mat4 viewTransform;
		mat4 projection;
		float rendererTime;
	} ubo;
	
	layout(push_constant) uniform pc {
		mat4 modelTransform;
	} push;

	void main() {
		gl_Position = ubo.projection * ubo.viewTransform * push.modelTransform * vec4(inPosition, 1.0);
		//gl_Position = push.modelTransform * vec4(inPosition, 1.0);
		fragColor = inColor;
		UVcoord = inUV;
	}
	)" "\0";

	std::string fragmentShader = 
	R"(
	#version 450

	layout(location = 0) in vec3 fragColor;
	layout(location = 1) in vec2 UVcoord;

	layout(location = 0) out vec4 outColor;

	void main() {
		outColor = vec4(fragColor, 1.0);
		//outColor = vec4(UVcoord, 1.0, 1.0);
	}
	)" "\0";

	std::string missingFragShader =
	R"(
	#version 450

	layout(location = 0) in vec3 fragColor;
	layout(location = 1) in vec2 UVcoord;

	layout(location = 0) out vec4 outColor;

	vec3 checker(in float u, in float v, in float scale) {
	  float fmodResult = mod(floor(scale * u) + floor(scale * v), 2.0);
	  float fin = max(sign(fmodResult), 0.0);
	  return vec3(fin, fin, fin);
	}

	void main() {
		outColor = vec4(checker(UVcoord.x, UVcoord.y, 2), 1.0) * vec4(1.0, 0.0, 1.0, 1.0);
	}
	)" "\0";

	std::string circleFragShader =
	R"(
	
	#version 450

	layout(location = 0) in vec3 fragColor;
	layout(location = 1) in vec2 UVcoord;

	layout(location = 0) out vec4 outColor;

	void main() {
		vec2 shifted = UVcoord - 0.5;
		if(length(shifted) < (sin(rendererTime * 0.001) + 1) * 0.25)
			outColor = vec4(fragColor, 1);
		else
			outColor = vec4(0,0,0,0);
	}
	)" "\0";

	auto compiledVertShader = shaderCompiler.compileShader({vertShader}, Iridium::shader_type::vertex);

	auto compiledFragShader = shaderCompiler.compileShader({fragmentShader}, Iridium::shader_type::fragment); // default
	//auto compiledFragShader = shaderCompiler.compileShader({missingFragShader}, Iridium::shader_type::fragment); // missing texture
	//auto compiledFragShader = shaderCompiler.compileShader({circleFragShader}, Iridium::shader_type::fragment); // circle

	VkShaderModule vertShaderModule = Vulkan::createShaderModule(compiledVertShader, m_device);
	defer(vkDestroyShaderModule(m_device, vertShaderModule, nullptr));
	VkShaderModule fragShaderModule = Vulkan::createShaderModule(compiledFragShader, m_device);
	defer(vkDestroyShaderModule(m_device, fragShaderModule, nullptr));

	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = {
		vertShaderStageInfo, 
		fragShaderStageInfo
	};

	auto bindingDesc = Iridium::Renderer::vertex::getBindingDescription();
	auto attribDescs = Iridium::Renderer::vertex::getAttributeDescriptors();

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &bindingDesc;
	vertexInputInfo.vertexAttributeDescriptionCount = attribDescs.size();
	vertexInputInfo.pVertexAttributeDescriptions = attribDescs.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(m_swapchainExtent.width);
	viewport.height = static_cast<float>(m_swapchainExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor{};
	scissor.offset = {0, 0};
	scissor.extent = m_swapchainExtent;

	std::vector<VkDynamicState> dynamicStates = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
		VK_DYNAMIC_STATE_POLYGON_MODE_EXT
	};

	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.polygonMode = VK_POLYGON_MODE_LINE;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_NONE;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f;
	multisampling.pSampleMask = nullptr;
	multisampling.alphaToCoverageEnable = VK_FALSE;
	multisampling.alphaToOneEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask =	VK_COLOR_COMPONENT_R_BIT |
											VK_COLOR_COMPONENT_G_BIT |
											VK_COLOR_COMPONENT_B_BIT |
											VK_COLOR_COMPONENT_A_BIT ;
	colorBlendAttachment.blendEnable = VK_TRUE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;

	VkPushConstantRange range{};
	range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	range.offset = 0;
	range.size = sizeof(Iridium::Renderer::push_constants);

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &range;
	if(vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS)
		throw Iridium::Renderer::renderer_error("Failed to create pipeline layout");

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2; // number of shaders
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = nullptr;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.layout = m_pipelineLayout;
	pipelineInfo.renderPass = m_renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;

	if(vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipeline) != VK_SUCCESS)
		throw Iridium::Renderer::renderer_error("Failed to create graphics pipeline");
	
}

void Iridium::Renderer::renderer::createFramebuffers() {
	m_swapchainFrameBuffers.resize(m_swapchainImageViews.size());
	for(size_t iterator = 0; iterator < m_swapchainImageViews.size(); iterator++) {
		VkImageView attachments[] = {
			m_swapchainImageViews[iterator]
		};

		VkFramebufferCreateInfo frambufferInfo{};
		frambufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		frambufferInfo.renderPass = m_renderPass;
		frambufferInfo.attachmentCount = 1;
		frambufferInfo.pAttachments = attachments;
		frambufferInfo.width = m_swapchainExtent.width;
		frambufferInfo.height = m_swapchainExtent.height;
		frambufferInfo.layers = 1;
		if(vkCreateFramebuffer(m_device, &frambufferInfo, nullptr, &m_swapchainFrameBuffers[iterator]) != VK_SUCCESS)
			throw Iridium::Renderer::renderer_error("Failed to create framebuffer");
	}
}

void Iridium::Renderer::renderer::createCommandPool() {
	using enum Iridium::Vulkan::queue_family_indices::family_type;

	Iridium::Vulkan::queue_family_indices indices = Iridium::Vulkan::findQueueFamilies(m_physicalDevice, m_surface);

	VkCommandPoolCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	createInfo.queueFamilyIndex = indices.families[graphics];

	if(vkCreateCommandPool(m_device, &createInfo, nullptr, &m_commandPool) != VK_SUCCESS)
		throw Iridium::Renderer::renderer_error("Failed to create command pool.");

}

void Iridium::Renderer::renderer::createVertexBuffer() {
	size_t bufferSize = sizeof(decltype(m_vertices)::value_type) * m_vertices.size();
	VkBuffer stagingBuffer = VK_NULL_HANDLE;
	VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingMemory);
	
	void* data;
	vkMapMemory(m_device, stagingMemory, 0, bufferSize, 0, &data);
	std::memcpy(data, m_vertices.data(), bufferSize);
	vkUnmapMemory(m_device, stagingMemory);
	
	createBuffer(bufferSize,  VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_vertexBuffer, m_vertexBufferMemory);

	copyBuffer(stagingBuffer, m_vertexBuffer, bufferSize);
	vkFreeMemory(m_device, stagingMemory, nullptr);
	vkDestroyBuffer(m_device, stagingBuffer, nullptr);
}

void Iridium::Renderer::renderer::createIndexBuffer() {
	size_t bufferSize = sizeof(decltype(m_indices)::value_type) * m_indices.size();
	VkBuffer stagingBuffer = VK_NULL_HANDLE;
	VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingMemory);
	
	void* data;
	vkMapMemory(m_device, stagingMemory, 0, bufferSize, 0, &data);
	std::memcpy(data, m_indices.data(), bufferSize);
	vkUnmapMemory(m_device, stagingMemory);
	
	createBuffer(bufferSize,  VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_indexBuffer, m_indexBufferMemory);

	copyBuffer(stagingBuffer, m_indexBuffer, bufferSize);
	vkFreeMemory(m_device, stagingMemory, nullptr);
	vkDestroyBuffer(m_device, stagingBuffer, nullptr);
}

void Iridium::Renderer::renderer::createUniformBuffers() {
	size_t bufferSize = sizeof(uniform_buffer);

	m_uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	m_uniformBuffersMapping.resize(MAX_FRAMES_IN_FLIGHT);
	m_uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);

	for(auto [buffer, mapping, memory] : std::views::zip(m_uniformBuffers, m_uniformBuffersMapping, m_uniformBuffersMemory)) {
		createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, buffer, memory);
		vkMapMemory(m_device, memory, 0, bufferSize, 0, &mapping);
	}
}

void Iridium::Renderer::renderer::cleanupVertexBuffer() {
	vkFreeMemory(m_device, m_vertexBufferMemory, nullptr);
	vkDestroyBuffer(m_device, m_vertexBuffer, nullptr);
}

void Iridium::Renderer::renderer::cleanupIndexBuffer() {
	vkFreeMemory(m_device, m_indexBufferMemory, nullptr);
	vkDestroyBuffer(m_device, m_indexBuffer, nullptr);
}

void Iridium::Renderer::renderer::cleanupUniformBuffers() {
	for(auto [buffer, memory] : std::views::zip(m_uniformBuffers, m_uniformBuffersMemory)) {
		vkFreeMemory(m_device, memory, nullptr);
		vkDestroyBuffer(m_device, buffer, nullptr);
	}
}

void Iridium::Renderer::renderer::createDescriptorPool() {
	VkDescriptorPoolSize poolSize{};
	poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSize.descriptorCount = MAX_FRAMES_IN_FLIGHT;

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = 1;
	poolInfo.pPoolSizes = &poolSize;
	poolInfo.maxSets = MAX_FRAMES_IN_FLIGHT;

	if(vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS)
		throw Iridium::Renderer::renderer_error("Failed to create descriptor pool.");
}

void Iridium::Renderer::renderer::createDescriptorSets() {
	std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, m_descriptorSetLayout);
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_descriptorPool;
	allocInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
	allocInfo.pSetLayouts = layouts.data();

	m_descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
	if(vkAllocateDescriptorSets(m_device, &allocInfo, m_descriptorSets.data()) != VK_SUCCESS)
		throw Iridium::Renderer::renderer_error("Failed to allocate descriptor sets.");

	for(auto [index, descriptorSet] : std::views::enumerate(m_descriptorSets)) {
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = m_uniformBuffers[index];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(uniform_buffer);

		VkWriteDescriptorSet descriptorWrite{};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = descriptorSet;
		descriptorWrite.dstBinding = 0;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pBufferInfo = &bufferInfo;
		descriptorWrite.pImageInfo = nullptr;
		descriptorWrite.pTexelBufferView = nullptr;
		vkUpdateDescriptorSets(m_device, 1, &descriptorWrite, 0, nullptr);
	}
}

void Iridium::Renderer::renderer::updateUniformBuffer(uint32_t currentImage) {
	uniform_buffer ubo{};
	ubo.projection = glm::perspective(glm::radians(45.0f), m_swapchainExtent.width / (float)m_swapchainExtent.height, 0.1f, 10.0f);
	ubo.rendererTime = std::chrono::duration<float, std::chrono::seconds::period>(std::chrono::steady_clock::now() - m_rendererStart).count();
	ubo.viewTransform = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.projection[1][1] *= -1.0f;
	memcpy(m_uniformBuffersMapping[currentImage], &ubo, sizeof(uniform_buffer));
}

void Iridium::Renderer::renderer::createCommandBuffers() {
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = m_commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;
	if(vkAllocateCommandBuffers(m_device, &allocInfo, m_commandBuffers) != VK_SUCCESS)
		throw Iridium::Renderer::renderer_error("Failed to allocate command buffers");
}

void Iridium::Renderer::renderer::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0;
	beginInfo.pInheritanceInfo = nullptr;
	if(vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
		throw Iridium::Renderer::renderer_error("Failed to begin recording command buffer");
	
	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = m_renderPass;
	renderPassInfo.framebuffer = m_swapchainFrameBuffers[imageIndex];
	renderPassInfo.renderArea.offset = {0, 0};
	renderPassInfo.renderArea.extent = m_swapchainExtent;
	
	VkClearValue clearColor = {{{0.05f, 0.05f, 0.07f, 1.0f}}};
	renderPassInfo.clearValueCount = 1;
	renderPassInfo.pClearValues = &clearColor;
	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(m_swapchainExtent.width);
	viewport.height = static_cast<float>(m_swapchainExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = {0, 0};
	scissor.extent = m_swapchainExtent;
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	VkPolygonMode mode = drawWireframe ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
	Vulkan::CmdSetPolygonModeEXT(m_instance, commandBuffer, mode);

	VkBuffer vertexBuffers[] = {m_vertexBuffer};
	VkDeviceSize offsets[] = {0};
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);

	float time = std::chrono::duration<float, std::chrono::seconds::period>(std::chrono::steady_clock::now() - m_rendererStart).count();
	push_constants constants{
		.modelTransform = glm::rotate(glm::mat4(1.0f), glm::degrees(1.0f) * time * 0.01f, glm::vec3(0.0f, 0.0f, 1.0f))
	};
	vkCmdPushConstants(commandBuffer, m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(push_constants), &constants);

	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptorSets[m_currentFrame], 0, nullptr);
	
	//vkCmdDraw(commandBuffer, m_vertices.size(), 1, 0, 0);
	vkCmdDrawIndexed(commandBuffer, m_indices.size(), 1, 0, 0, 0);
	
	vkCmdEndRenderPass(commandBuffer);
	if(vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
		throw Iridium::Renderer::renderer_error("Failed to record command buffer");
}

void Iridium::Renderer::renderer::createSyncObjects() {
	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for(size_t iterator = 0; iterator < MAX_FRAMES_IN_FLIGHT; iterator++) {
		if(vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[iterator]) != VK_SUCCESS)
			throw Iridium::Renderer::renderer_error("Failed to create image available semaphore.");
		
		if(vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[iterator]) != VK_SUCCESS)
			throw Iridium::Renderer::renderer_error("Failed to create render finished semaphore.");
		
		if(vkCreateFence(m_device, &fenceInfo, nullptr, &m_inFlightFences[iterator]) != VK_SUCCESS)
			throw Iridium::Renderer::renderer_error("Failed to create in-flight fence.");
		
		if(vkCreateFence(m_device, &fenceInfo, nullptr, &m_presentFences[iterator]) != VK_SUCCESS)
			throw Iridium::Renderer::renderer_error("Failed to create present fence.");
	}
}

void Iridium::Renderer::renderer::destroySyncObjects() {
	for(size_t iterator = 0; iterator < MAX_FRAMES_IN_FLIGHT; iterator++) {
		vkDestroySemaphore(m_device, m_imageAvailableSemaphores[iterator], nullptr);
		vkDestroySemaphore(m_device, m_renderFinishedSemaphores[iterator], nullptr);
		vkDestroyFence(m_device, m_inFlightFences[iterator], nullptr);
		vkDestroyFence(m_device, m_presentFences[iterator], nullptr);
	}
}

void Iridium::Renderer::renderer::drawFrame() {
	vkWaitForFences(m_device, 1, &m_presentFences[m_currentFrame], VK_TRUE, UINT64_MAX);
	vkResetFences(m_device, 1, &m_presentFences[m_currentFrame]);

	vkWaitForFences(m_device, 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);
	uint32_t imageIndex = 0;
	VkResult result = vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX, m_imageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE, &imageIndex);
	if(result == VK_ERROR_OUT_OF_DATE_KHR) {
		recreateSwapchain();
		return;
	} else if(result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw Iridium::Renderer::renderer_error("Failed to acquire swap chain image");
	}
	vkResetFences(m_device, 1, &m_inFlightFences[m_currentFrame]);
	
	vkResetCommandBuffer(m_commandBuffers[m_currentFrame], 0);
	recordCommandBuffer(m_commandBuffers[m_currentFrame], imageIndex);
	
	VkSemaphore waitSemaphores[] = { m_imageAvailableSemaphores[m_currentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSemaphore signalSemaphores[] = { m_renderFinishedSemaphores[m_currentFrame] };

	updateUniformBuffer(m_currentFrame);
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_commandBuffers[m_currentFrame];
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;
	if(vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_inFlightFences[m_currentFrame]) != VK_SUCCESS)
		throw std::runtime_error("failed to submit draw command buffer");

	VkSwapchainKHR swapChains[] = { m_swapchain };

	VkSwapchainPresentFenceInfoEXT fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_FENCE_INFO_EXT;
	fenceInfo.swapchainCount = 1;
	fenceInfo.pFences = &m_presentFences[m_currentFrame];

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr;
	presentInfo.pNext = &fenceInfo;

	result = vkQueuePresentKHR(m_presentQueue, &presentInfo);
	if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_framebufferResized) {
		recreateSwapchain();
		m_framebufferResized = false;
	} else if(result != VK_SUCCESS) {
		throw Iridium::Renderer::renderer_error("failed to present swap chain image");
	}

	m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

uint32_t Iridium::Renderer::renderer::findMemoryType(uint32_t filter, VkMemoryPropertyFlags properties) {
	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memoryProperties);

	for(uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
		if(filter & (1 << i) && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	throw Iridium::Renderer::renderer_error("Failed to find suitable memory type.");
}

void Iridium::Renderer::renderer::createBuffer(size_t size, VkBufferUsageFlags flags, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory &memory) {
	using enum Iridium::Vulkan::queue_family_indices::family_type;
	
	auto queueFamilies = Iridium::Vulkan::findQueueFamilies(m_physicalDevice, m_surface);
	std::vector<uint32_t> indices;
	VkSharingMode mode = VK_SHARING_MODE_MAX_ENUM;
	if(queueFamilies.families[transfer] != queueFamilies.families[graphics]) {
		indices.push_back(queueFamilies.families[transfer]);
		indices.push_back(queueFamilies.families[graphics]);
		mode = VK_SHARING_MODE_CONCURRENT;
	} else {
		indices.push_back(queueFamilies.families[graphics]);
		mode = VK_SHARING_MODE_EXCLUSIVE;
	}
	
	VkBufferCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	createInfo.usage = flags;
	createInfo.size = size;
	createInfo.sharingMode = mode;
	createInfo.pQueueFamilyIndices = indices.data();
	createInfo.queueFamilyIndexCount = indices.size();

	if(vkCreateBuffer(m_device, &createInfo, nullptr, &buffer) != VK_SUCCESS) {
		throw Iridium::Renderer::renderer_error("Failed to create a buffer.");
	}

	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(m_device, buffer, &memoryRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memoryRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memoryRequirements.memoryTypeBits, properties);

	if(vkAllocateMemory(m_device, &allocInfo, nullptr, &memory) != VK_SUCCESS) {
		throw Iridium::Renderer::renderer_error("Failed to allocate memory buffer.");
	}
	vkBindBufferMemory(m_device, buffer, memory, 0);
}

void Iridium::Renderer::renderer::copyBuffer(VkBuffer src, VkBuffer dst, size_t size) {
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = m_commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	VkBufferCopy copyRegion{};
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, src, dst, 1, &copyRegion);

	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(m_graphicsQueue);
	vkFreeCommandBuffers(m_device, m_commandPool, 1, &commandBuffer);
}

Iridium::Renderer::renderer* Iridium::Renderer::getRenderer() {
	return getApplicationPointer()->renderer;
}