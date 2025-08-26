#include "entryPoint.hpp"
#include <cstddef>
#include <cstdint>
#include <exception>
#include <print>
#include <set>
#include <stdexcept>
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>

#include "log.hpp"
#include "renderer/vulkan.hpp"
#include "renderer/shader.hpp"
#include "thread.hpp"
#include "utils.hpp"

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

namespace ir = Iridium;

extern ir::application& createApp();

VkResult CreateDebugUtilsMessengerEXT(
	VkInstance instance,
	const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkDebugUtilsMessengerEXT* pDebugMessenger) {
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
		instance,
		"vkCreateDebugUtilsMessengerEXT");
	if(func) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	} else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void DestroyDebugUtilsMessengerEXT(
	VkInstance instance,
	VkDebugUtilsMessengerEXT debugMessenger,
	const VkAllocationCallbacks* pAllocator) {
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
			instance,
			"vkDestroyDebugUtilsMessengerEXT");
		if(func)
			func(instance, debugMessenger, pAllocator);
}

bool isDeviceSuitable([[maybe_unused]] VkPhysicalDevice device) {
	return true;
}

class HelloTriangleApplicaiton {
public:
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback__(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		[[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		[[maybe_unused]] void* pUserData
	) {
		if(messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
			std::println("vk validation layer: {}", pCallbackData->pMessage);
		}
		return VK_FALSE;
	}

	void run() {
		initWindow();
		initVulkan();
		mainLoop();
		cleanup();
	}
private:
	enum constants {
		MAX_FRAMES_IN_FLIGHT = 2
	};

	GLFWwindow* m_window = nullptr;

	VkInstance m_instance = VK_NULL_HANDLE;
	VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
	VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
	VkDevice m_device = VK_NULL_HANDLE;
	VkQueue m_graphicsQueue = VK_NULL_HANDLE;
	VkQueue m_computeQueue = VK_NULL_HANDLE;
	VkQueue m_presentQueue = VK_NULL_HANDLE;
	VkSurfaceKHR m_surface = VK_NULL_HANDLE;
	VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
	std::vector<VkImage> m_swapchainImages{};
	VkFormat m_swapChainImageFormat{};
	VkExtent2D m_swapChainExtent{};
	std::vector<VkImageView> m_swapchainImageViews{};
	VkRenderPass m_renderPass = VK_NULL_HANDLE;
	VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
	VkPipeline m_graphicsPipeline = VK_NULL_HANDLE;
	std::vector<VkFramebuffer> m_swapchainFrameBuffers{};
	VkCommandPool m_commandPool = VK_NULL_HANDLE;
	
	//VkCommandBuffer m_commandBuffer;
	//VkSemaphore m_imageAvailableSemaphore;
	//VkSemaphore m_renderFinishedSemaphore;
	//VkFence m_inFlightFence;

	uint32_t m_currentFrame = 0;
	VkCommandBuffer m_commandBuffers[MAX_FRAMES_IN_FLIGHT]{};
	VkSemaphore m_imageAvailableSemaphores[MAX_FRAMES_IN_FLIGHT]{};
	VkSemaphore m_renderFinishedSemaphores[MAX_FRAMES_IN_FLIGHT]{};
	VkFence m_inFlightFences[MAX_FRAMES_IN_FLIGHT]{};
	bool m_framebufferResized = false;

	ir::shader_compiler m_shaderCompiler;

	void createInstance() {
		if(USE_VALIDATION_LAYERS && !ir::checkValidationLayers()) {
			throw ir::Renderer::renderer_error("validation layers requested, but not available.");
		}

		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pNext = nullptr;
		appInfo.pApplicationName = "IridiumEngine";
		appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
		appInfo.pEngineName = "Iridium";
		appInfo.engineVersion = VK_MAKE_VERSION(0, 0, 1);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		auto extensions = ir::getRequiredExtensions();
		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;
		createInfo.enabledLayerCount = 0;
		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();

		auto& validationLayers = ir::getValidationLayers();
		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
		if constexpr(USE_VALIDATION_LAYERS) {
			ir::populateVkDeugUtilsMessengerCreateInfoEXT(debugCreateInfo);
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
			createInfo.pNext = &debugCreateInfo;
		}
		
		VkResult createResult = vkCreateInstance(&createInfo, nullptr, &m_instance);
		if(createResult != VK_SUCCESS) {
			throw ir::Renderer::renderer_error(std::format("Failed to create vk instance with error code: {}", (size_t)createResult));
		}
	}

	void initWindow() {
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		//glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		m_window = glfwCreateWindow(800, 600, "vkWindow", nullptr, nullptr);
		glfwSetWindowUserPointer(m_window, this);
		glfwSetFramebufferSizeCallback(m_window, [](GLFWwindow* window, int, int) {
			auto& app = *reinterpret_cast<HelloTriangleApplicaiton*>(glfwGetWindowUserPointer(window));
			app.m_framebufferResized = true;
		});
	}

	void initVulkan() {
		createInstance();
		setupDebugMesenger();
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

	void createSyncObjects() {
		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for(size_t iterator = 0; iterator < MAX_FRAMES_IN_FLIGHT; iterator++) {
		if(vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[iterator]) != VK_SUCCESS)
			throw ir::Renderer::renderer_error("failed to create image semaphore");
		if(vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[iterator]) != VK_SUCCESS)
			throw ir::Renderer::renderer_error("failed to create render semaphore");
		if(vkCreateFence(m_device, &fenceInfo, nullptr, &m_inFlightFences[iterator]) != VK_SUCCESS)
			throw ir::Renderer::renderer_error("failed to create inFlight fence");
		}
	}

	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0;
		beginInfo.pInheritanceInfo = nullptr;
		if(vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
			throw ir::Renderer::renderer_error("failed to begin recording command buffer");

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = m_renderPass;
		renderPassInfo.framebuffer = m_swapchainFrameBuffers[imageIndex];
		renderPassInfo.renderArea.offset = {0, 0};
		renderPassInfo.renderArea.extent = m_swapChainExtent;
		
		VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 0.0f}}};
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;
		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		//Recording command buffers start
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(m_swapChainExtent.width);
		viewport.height = static_cast<float>(m_swapChainExtent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = {0, 0};
		scissor.extent = m_swapChainExtent;
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
		vkCmdDraw(commandBuffer, 3, 1, 0, 0);;

		vkCmdEndRenderPass(commandBuffer);

		//End recording command buffer
		if(vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
			throw ir::Renderer::renderer_error("failed to record command buffer");
	}

	void createCommandBuffers() {
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = m_commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;
		if(vkAllocateCommandBuffers(m_device, &allocInfo, m_commandBuffers) != VK_SUCCESS)
			throw ir::Renderer::renderer_error("failed to alloc command buffer");
	}

	void createCommandPool() {
		ir::queue_family_indices queueFamilyIndices = ir::findQueueFamilies(m_physicalDevice, m_surface);
		
		VkCommandPoolCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		createInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
		if(vkCreateCommandPool(m_device, &createInfo, nullptr, &m_commandPool) != VK_SUCCESS)
			throw ir::Renderer::renderer_error("failed to create command pool");
	}

	void createFramebuffers() {
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
			frambufferInfo.width = m_swapChainExtent.width;
			frambufferInfo.height = m_swapChainExtent.height;
			frambufferInfo.layers = 1;
			if(vkCreateFramebuffer(m_device, &frambufferInfo, nullptr, &m_swapchainFrameBuffers[iterator]) != VK_SUCCESS)
				throw ir::Renderer::renderer_error("failed to create framebuffer");
		}
	}

	VkShaderModule createShaderModule(const std::vector<uint32_t>& compiledShader) {
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = compiledShader.size() * sizeof(uint32_t);
		createInfo.pCode = compiledShader.data();

		VkShaderModule shaderModule;
		if(vkCreateShaderModule(m_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
			throw ir::Renderer::renderer_error("failed to create shader module");
		return shaderModule;
	}

	void createGraphicsPipeline() {
		std::string vertexShader = 
		R"(
		#version 450

		vec2 positions[3] = vec2[] (
			vec2( 0.0, -0.5),	
			vec2( 0.5,  0.5),
			vec2(-0.5,  0.5)
		);

		vec3 colors[3] = vec3[] (
			vec3(1.0, 0.0, 0.0),
			vec3(0.0, 1.0, 0.0),
			vec3(0.0, 0.0, 1.0)		
		);

		layout(location = 0) out vec3 fragColor;

		void main() {
			gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
			fragColor = colors[gl_VertexIndex];
		}
		)" "\0";
		auto compiledVertexShader = m_shaderCompiler.compileShader({vertexShader}, ir::shader_type::vertex);

		std::string fragmentShader = 
		R"(
		#version 450

		layout(location = 0) in vec3 fragColor;

		layout(location = 0) out vec4 outColor;

		void main() {
			outColor = vec4(fragColor, 1.0);
		}
		)"  "\0";

		auto compiledFragmentShader = m_shaderCompiler.compileShader({fragmentShader}, ir::shader_type::fragment);

		VkShaderModule vertexShaderModule = createShaderModule(compiledVertexShader);
		defer(vkDestroyShaderModule(m_device, vertexShaderModule, nullptr););
		VkShaderModule fragmentShaderModule = createShaderModule(compiledFragmentShader);
		defer(vkDestroyShaderModule(m_device, fragmentShaderModule, nullptr););

		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertexShaderModule;
		vertShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragmentShaderModule;
		fragShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 0;
		vertexInputInfo.pVertexBindingDescriptions = nullptr;
		vertexInputInfo.vertexAttributeDescriptionCount = 0;
		vertexInputInfo.pVertexAttributeDescriptions = nullptr;

		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(m_swapChainExtent.width);
		viewport.height = static_cast<float>(m_swapChainExtent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor{};
		scissor.offset = {0, 0};
		scissor.extent = m_swapChainExtent;

		std::vector<VkDynamicState> dynamicStates = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
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
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
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

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 0;
		pipelineLayoutInfo.pSetLayouts = nullptr;
		pipelineLayoutInfo.pushConstantRangeCount = 0;
		pipelineLayoutInfo.pPushConstantRanges = nullptr;
		if(vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS)
			throw ir::Renderer::renderer_error("failed to create pipeline layout");

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
			throw ir::Renderer::renderer_error("failed to create graphics pipeline");
	}

	void createRenderPass() {
		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = m_swapChainImageFormat;
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
			throw ir::Renderer::renderer_error("failed to create render pass");
	}

	void createImageViews() {
		m_swapchainImageViews.resize(m_swapchainImages.size());
		size_t iterator = 0;
		for (const auto& image : m_swapchainImages) {
			VkImageViewCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = image;
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format = m_swapChainImageFormat;
			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.layerCount = 1;
			if(vkCreateImageView(m_device, &createInfo, nullptr, &m_swapchainImageViews[iterator]) != VK_SUCCESS) {
				throw ir::Renderer::renderer_error("failed to create image views.");
			}
			iterator++;
		}
	}

	void recreateSwapchain() {
		int width, height;
		glfwGetFramebufferSize(m_window, &width, &height);
		while(width == 0 || height == 0) {
			glfwGetFramebufferSize(m_window, &width, &height);
			glfwWaitEvents();
		}
		vkDeviceWaitIdle(m_device);

		cleanupSwapchain();

		createSwapchain();
		createImageViews();
		createFramebuffers();
	}

	void cleanupSwapchain() {
		for(auto& framebuffer : m_swapchainFrameBuffers) {
			vkDestroyFramebuffer(m_device, framebuffer, nullptr);
		}
		for(auto imageView : m_swapchainImageViews) {
			vkDestroyImageView(m_device, imageView, nullptr);
		}
		vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
	}

	void createSwapchain() {
		ir::swapchain_support_details swapchainSupport = ir::querySwapchainSupport(m_physicalDevice, m_surface);
		VkSurfaceFormatKHR surfaceFormat = ir::chooseSwapSurfaceFormat(swapchainSupport.formats);
		VkPresentModeKHR presentMode = ir::chooseSwapPresentMode(swapchainSupport.presentModes);
		VkExtent2D extent = ir::chooseSwapExtent(swapchainSupport.capabilities, m_window);
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

		ir::queue_family_indices indices = Iridium::findQueueFamilies(m_physicalDevice, m_surface);
		uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.computeFamily.value(), indices.presentFamily.value()};
		if(indices.graphicsFamily != indices.computeFamily || indices.computeFamily != indices.presentFamily || indices.presentFamily != indices.graphicsFamily) {
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
			throw ir::Renderer::renderer_error("failed to create swapchain");

		vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, nullptr);
		m_swapchainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, m_swapchainImages.data());
	
		m_swapChainImageFormat = surfaceFormat.format;
		m_swapChainExtent = extent;
	}

	void createSurface() {
		if(glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface) != VK_SUCCESS) {
			throw ir::Renderer::renderer_error("failed to create window surface");
		}
	}

	void createLogicalDevice() {
		ir::queue_family_indices indices = ir::findQueueFamilies(m_physicalDevice, m_surface);
		
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t> uniqueQueueFamilies = {
			indices.graphicsFamily.value(),
			indices.computeFamily.value(),
			indices.presentFamily.value()
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

		auto& deviceExtensions = ir::getDeviceExtensions();
		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pEnabledFeatures = &deviceFeatures;
		createInfo.ppEnabledExtensionNames = deviceExtensions.data();
		createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
		if constexpr(USE_VALIDATION_LAYERS) {
			auto& validationLayers = ir::getValidationLayers();
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		} else {
			createInfo.enabledLayerCount = 0;
		}

		if(vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device) != VK_SUCCESS) {
			throw ir::Renderer::renderer_error("failed to create logical device.");
		}
		vkGetDeviceQueue(m_device, indices.graphicsFamily.value(), 0, &m_graphicsQueue);
		vkGetDeviceQueue(m_device, indices.computeFamily.value(), 0, &m_computeQueue);
		vkGetDeviceQueue(m_device, indices.presentFamily.value(), 0, &m_presentQueue);
	}

	void pickPhysicalDevice() {
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);
		if(deviceCount == 0)
			throw ir::Renderer::renderer_error("Failed to find suitable gpu.");

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

		for(const auto& device : devices) {
			if(isDeviceSuitable(device)) {
				m_physicalDevice = device;
				break;
			}
		}

		if(m_physicalDevice == VK_NULL_HANDLE)
			throw ir::Renderer::renderer_error("Failed to find suitable gpu.");

		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(m_physicalDevice, &properties);

		ENGINE_LOG_INFO("Device name: {}", properties.deviceName);
	}

	void setupDebugMesenger() {
		if constexpr(!USE_VALIDATION_LAYERS) return;

		VkDebugUtilsMessengerCreateInfoEXT createInfo{};
		ir::populateVkDeugUtilsMessengerCreateInfoEXT(createInfo);

		VkResult result = CreateDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, &m_debugMessenger);
		if(result != VK_SUCCESS) {
			throw std::runtime_error("Failed to set up debug messenger.");
		}
	}

	void mainLoop() {
		while(!glfwWindowShouldClose(m_window)) {
			glfwPollEvents();
			drawFrame();
		}
		vkDeviceWaitIdle(m_device);
	}

	void drawFrame() {
		vkWaitForFences(m_device, 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);
		
		uint32_t imageIndex = 0;
		VkResult result = vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX, m_imageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE, &imageIndex);
		if(result == VK_ERROR_OUT_OF_DATE_KHR) {
			recreateSwapchain();
			return;
		} else if(result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw ir::Renderer::renderer_error("failed to acquire swap chain image");
		}
		vkResetFences(m_device, 1, &m_inFlightFences[m_currentFrame]);

		vkResetCommandBuffer(m_commandBuffers[m_currentFrame], 0);
		recordCommandBuffer(m_commandBuffers[m_currentFrame], imageIndex);
		
		VkSemaphore waitSemaphores[] = { m_imageAvailableSemaphores[m_currentFrame] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		VkSemaphore signalSemaphores[] = { m_renderFinishedSemaphores[m_currentFrame] };

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

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr;

		result = vkQueuePresentKHR(m_presentQueue, &presentInfo);
		if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_framebufferResized) {
			recreateSwapchain();
			m_framebufferResized = false;
		} else if(result != VK_SUCCESS) {
			throw ir::Renderer::renderer_error("failed to present swap chain image");
		}

		m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	void cleanup() {

		for(size_t iterator = 0; iterator < MAX_FRAMES_IN_FLIGHT; iterator++) {
			vkDestroyFence(m_device, m_inFlightFences[iterator], nullptr);
			vkDestroySemaphore(m_device, m_imageAvailableSemaphores[iterator], nullptr);
			vkDestroySemaphore(m_device, m_renderFinishedSemaphores[iterator], nullptr);
		}
		vkDestroyCommandPool(m_device, m_commandPool, nullptr);
		cleanupSwapchain();
		vkDestroyPipeline(m_device, m_graphicsPipeline, nullptr);
		vkDestroyPipelineLayout(m_device,m_pipelineLayout,nullptr);
		vkDestroyRenderPass(m_device, m_renderPass, nullptr);
		vkDestroyDevice(m_device, nullptr);
		if constexpr(USE_VALIDATION_LAYERS)
			DestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
		vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
		vkDestroyInstance(m_instance, nullptr);

		glfwDestroyWindow(m_window);
		glfwTerminate();
	}
};

extern void entryPoint();

int main(int argc, char** argv) {
	ir::setThreadName("Main");

	ENGINE_LOG_INFO("Args are:");
	for(int iterator = 0; iterator < argc; iterator++) {
		ENGINE_LOG_INFO_NP("#{}>{}", iterator, argv[iterator]);
	}

	try {
		HelloTriangleApplicaiton app;
		app.run();
	} catch (std::exception& e) {
		ENGINE_LOG_ERROR("{}", std::string_view(e.what()));
	}
}

int test() {
	return 42;
}