#pragma once
#include "glm/fwd.hpp"
#include <glm/glm.hpp>
#include <vulkan/vulkan_core.h>

#include <array>

namespace Iridium {
	namespace Renderer {
		struct vertex {
			glm::vec3 position;
			glm::vec3 color;
			glm::vec2 uv;

			static VkVertexInputBindingDescription getBindingDescription() {
				VkVertexInputBindingDescription bindingDesc{};
				bindingDesc.binding = 0;
				bindingDesc.stride = sizeof(vertex);
				bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
				return bindingDesc;
			}

			static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptors() {
				std::array<VkVertexInputAttributeDescription, 3> descriptors{};

				descriptors[0].binding = 0;
				descriptors[0].location = 0;
				descriptors[0].format = VK_FORMAT_R32G32B32_SFLOAT;
				descriptors[0].offset = offsetof(vertex, position);

				descriptors[1].binding = 0;
				descriptors[1].location = 1;
				descriptors[1].format = VK_FORMAT_R32G32B32_SFLOAT;
				descriptors[1].offset = offsetof(vertex, color);

				descriptors[2].binding = 0;
				descriptors[2].location = 2;
				descriptors[2].format = VK_FORMAT_R32G32_SFLOAT;
				descriptors[2].offset = offsetof(vertex, uv);

				return descriptors;
			}
		};
	}
}