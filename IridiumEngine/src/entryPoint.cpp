#include "entryPoint.hpp"
#include <chrono>
#include <exception>
#include <iterator>
#include <ranges>
#include <thread>
#include <vulkan/vulkan_core.h>

#include "appinfo.hpp"
#include "inputHandler.hpp"
#include "log.hpp"
#include "renderer/renderer.hpp"
#include "assets/shaderCompiler.hpp"
#include "renderer/window.hpp"
#include "thread.hpp"
#include "utils.hpp"

#ifdef _MSC_VER
#include <Windows.h>
#endif

namespace Ir = Iridium;

extern Ir::application& createApp();

Ir::application::application(Iridium::appinfo& info) {
	setApplicationPointer(this, I_KNOW_WHAT_I_AM_DOING);

	threadManager = ::new thread_manager();
	windowManager = ::new window_manager();
	windowManager->createWindow(800, 600, info.name);
	inputHandler = ::new input_handler();
	shaderCompiler = ::new shader_compiler();
	renderer = ::new Renderer::renderer(info);
	threadManager->spawnThread("Render", [&]() -> void {
		ENGINE_LOG_INFO("Starting render thread.");
		while(!windowManager->windowShouldClose())
			renderer->drawFrame();
	});

	while(!windowManager->windowShouldClose()) {
		windowManager->pollEvents();
		glm::vec3 moveVector{};
		if(getInputHandler()->isKeyPressed(KEY_W)) {
			moveVector += glm::vec3(1.0, 0.0, 0.0);
		}
		if(getInputHandler()->isKeyPressed(KEY_S)) {
			moveVector += glm::vec3(-1.0, 0.0, 0.0);
		}
		if(getInputHandler()->isKeyPressed(KEY_A)) {
			moveVector += glm::vec3(0.0, 1.0, 0.0);
		}
		if(getInputHandler()->isKeyPressed(KEY_D)) {
			moveVector += glm::vec3(0.0, -1.0, 0.0);
		}
		if(getInputHandler()->isKeyPressed(KEY_SPACE)) {
			moveVector += glm::vec3(0.0, 0.0, 1.0);
		}
		if(getInputHandler()->isKeyPressed(KEY_RCONTROL) || getInputHandler()->isKeyPressed(KEY_LCONTROL)) {
			moveVector += glm::vec3(0.0, 0.0, -1.0);
		}
		if(glm::length(moveVector)) {
			moveVector = glm::normalize(moveVector);
		}

		static bool heldR = false;
		if(getInputHandler()->isKeyPressed(KEY_R)) {
			if(!heldR)
				renderer->drawWireframe = !renderer->drawWireframe;
			heldR = true;
		} else {
			heldR = false;
		}

		static bool heldEnt = false;
		if(getInputHandler()->isKeyPressed(KEY_ENTER)) {
			if(!heldEnt) {
				auto unicodeText = getInputHandler()->getTextInputAndClear();
				std::vector<char> utf8Text = unicodeToUTF8(unicodeText);
				utf8Text.push_back('\0');
				ENGINE_LOG_ERROR("{}", utf8Text.data());
			}
			heldEnt = true;
		} else {
			heldEnt = false;
		}

		moveVector *= 0.01f;
		renderer->setCameraPos(renderer->getCameraPos() + moveVector);
		//std::this_thread::sleep_for(std::chrono::seconds(1));
	};
}

extern void entryPoint();

int main(int argc, char** argv) {
#ifdef _MSC_VER
	SetConsoleOutputCP(65001);
#endif

	Ir::setThreadName("Main");
	auto span = std::span(argv, std::next(argv, argc));
	ENGINE_LOG_INFO("Argumets are:");
	for(auto [index, option] : std::views::enumerate(span)) {
		ENGINE_LOG_INFO_NP("#{} -> {}", index, option);
	}

	try {
		[[maybe_unused]] Iridium::application& app = createApp();
	} catch (std::exception& e) {
		ENGINE_LOG_FATAL("Oh Fiddlesticks! What now?");
		ENGINE_LOG_FATAL_NP("{}", e.what());
	}
	return 0;
}