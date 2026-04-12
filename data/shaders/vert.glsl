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