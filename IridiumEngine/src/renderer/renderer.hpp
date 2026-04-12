#pragma once

#include <chrono>
#include <cstdint>
#include <vector>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

#include "../appinfo.hpp"
#include "vertex.hpp"
#include "window.hpp"

namespace Iridium {
	namespace Renderer {
		struct push_constants {
			glm::mat4 modelTransform;
		};

		struct uniform_buffer {
			glm::mat4 viewTransform;
			glm::mat4 projection;
			float rendererTime;
		};

		class renderer {
		public:
			renderer(appinfo& info);

			void inline testLoop() {
				while(!getWindowManager()->windowShouldClose()) {
					glfwPollEvents();
					drawFrame();
				}
				//vkDeviceWaitIdle(m_device);
			}

			void inline cleanup() {
				cleanupVulkan();
			}

			bool drawWireframe = false;
		private:
			enum { //constants
				MAX_FRAMES_IN_FLIGHT = 3
			};
			
			const appinfo& m_info;

			//Vulkan
			VkInstance m_instance;
			VkDebugUtilsMessengerEXT m_debugMessenger;
			VkSurfaceKHR m_surface;
			VkPhysicalDevice m_physicalDevice;
			VkDevice m_device;

			VkQueue m_graphicsQueue;
			VkQueue m_computeQueue;
			VkQueue m_presentQueue;
			VkQueue m_transferQueue;
			
			VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
			std::vector<VkImage> m_swapchainImages;
			VkFormat m_swapchainImageFormat;
			VkExtent2D m_swapchainExtent;
			std::vector<VkImageView> m_swapchainImageViews;
			VkRenderPass m_renderPass;
			VkDescriptorSetLayout m_descriptorSetLayout;
			VkPipelineLayout m_pipelineLayout;
			VkPipeline m_graphicsPipeline;
			std::vector<VkFramebuffer> m_swapchainFrameBuffers;
			VkCommandPool m_commandPool;
			VkCommandBuffer m_commandBuffers[MAX_FRAMES_IN_FLIGHT];
			
			VkSemaphore m_imageAvailableSemaphores[MAX_FRAMES_IN_FLIGHT];
			VkSemaphore m_renderFinishedSemaphores[MAX_FRAMES_IN_FLIGHT];
			VkFence m_inFlightFences[MAX_FRAMES_IN_FLIGHT];
			VkFence m_presentFences[MAX_FRAMES_IN_FLIGHT];

			VkBuffer m_vertexBuffer;
			VkDeviceMemory m_vertexBufferMemory;
			
			VkBuffer m_indexBuffer;
			VkDeviceMemory m_indexBufferMemory;

			std::vector<VkBuffer> m_uniformBuffers;
			std::vector<VkDeviceMemory> m_uniformBuffersMemory;
			std::vector<void*> m_uniformBuffersMapping;

			VkDescriptorPool m_descriptorPool;
			std::vector<VkDescriptorSet> m_descriptorSets;

			bool m_framebufferResized = false;
			uint16_t m_currentFrame = 0;

			std::chrono::steady_clock::time_point m_rendererStart;

			std::vector<Iridium::Renderer::vertex> m_vertices {
				{{-0.5f, -0.5f, 0.0f},{1, 0, 0}, {0, 0}},
				{{ 0.5f, -0.5f, 0.0f},{0, 1, 0}, {1, 0}},
				{{ 0.5f,  0.5f, 0.0f},{0, 0, 1}, {1, 1}},
				{{-0.5f,  0.5f, 0.0f},{1, 1, 1}, {0, 1}},


				{{-0.5f, -0.5f, -0.25f},{1, 0, 0}, {0, 0}},
				{{ 0.5f, -0.5f, -0.25f},{0, 1, 0}, {1, 0}},
				{{ 0.5f,  0.5f, -0.25f},{0, 0, 1}, {1, 1}},
				{{-0.5f,  0.5f, -0.25f},{1, 1, 1}, {0, 1}},
			};

			std::vector<uint32_t> m_indices {
				0, 1, 2,
				2, 3, 0,

				4, 5, 6,
				6, 7, 4,
			};
			
			void initVulkan();
			void cleanupVulkan();
			
			void createInstance();

			void setupDebugMessenger();

			void createSurface(); //depends on window

			void setWindowCallbacks();
			
			void pickPhysicalDevice();
			
			void createLogicalDevice();
			
			void createSwapchain();
			void recreateSwapchain();
			void cleanupSwapchain();

			void createImageViews();
			
			void createRenderPass();

			void createDescriptorSetLayout();
			void createDescriptorSets();
			
			void createGraphicsPipeline();
			
			void createFramebuffers();
			
			void createCommandPool();

			void createVertexBuffer();
			void createIndexBuffer();
			void createUniformBuffers();
			void cleanupVertexBuffer();
			void cleanupIndexBuffer();
			void cleanupUniformBuffers();

			void updateUniformBuffer(uint32_t);

			void createDescriptorPool();

			void createCommandBuffers();
			void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
			
			void createSyncObjects();
			void destroySyncObjects();

			void drawFrame();

			void cleanupGLFW();

			//helpers
			uint32_t findMemoryType(uint32_t filter, VkMemoryPropertyFlags properties);

			void createBuffer(size_t size, VkBufferUsageFlags flags, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory &memory);
			void copyBuffer(VkBuffer src, VkBuffer dst, size_t size);
		};

		renderer* getRenderer();
	}
}