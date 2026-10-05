#include "entryPoint.hpp"
#include <chrono>
#include <exception>
#include <iterator>
#include <ranges>
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
#include <timeapi.h>
#pragma comment(lib, "winmm.lib")
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
	auto lastFrameTime = std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(std::chrono::milliseconds(1));
	auto lastTickTime = std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(std::chrono::milliseconds(1));
	threadManager->spawnThread("Render", [&]() -> void {
		ENGINE_LOG_INFO("Starting render thread.");

		auto clock = std::chrono::steady_clock();
		size_t counter = 0;
		while(!windowManager->windowShouldClose()) {
			auto start = clock.now();
			renderer->drawFrame();	
			lastFrameTime = clock.now() - start;
			auto interpolationRatio = (lastFrameTime / lastTickTime);
			renderer->m_interpolationRatio = interpolationRatio;
			//ENGINE_LOG_ERROR("INTERPOLATION RATIO: {}", interpolationRatio);
			if(counter == 2000) {
				getWindowManager()->setWindowName(std::format("FPS: {}", 1.0f / std::chrono::duration_cast<std::chrono::duration<double>>(lastFrameTime).count()).c_str());
				counter = 0;
			}
			counter++;
		}
	});

	auto clock = std::chrono::steady_clock();
	size_t counter = 0;
	glm::vec3 pos{0.0f, 0.0f, 0.0f};
	while(!windowManager->windowShouldClose()) {
		auto start = clock.now();

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
				ENGINE_LOG_ERROR("{}", (char*)utf8Text.data());
			}
			heldEnt = true;
		} else {
			heldEnt = false;
		}

		moveVector *= 0.001f * lastTickTime.count();
		pos += moveVector;
		renderer->setCameraPos(pos);
		std::this_thread::sleep_for(std::chrono::milliseconds(50)); // No sleep causes weird issue
		lastTickTime = clock.now() - start;
		if(counter == 200) {
			ENGINE_LOG_INFO("TPS: {}", 1.0f / std::chrono::duration_cast<std::chrono::duration<double>>(lastTickTime).count());
			counter = 0;
		}
		counter++;
	};
}

extern void entryPoint();

int main(int argc, char** argv) {
#ifdef _MSC_VER
	SetConsoleOutputCP(65001);
	timeBeginPeriod(1);
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

#ifdef _MSC_VER
	timeEndPeriod(1);
#endif
	return 0;
}