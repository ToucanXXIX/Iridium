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
	
	threadManager->spawnThread("Render", [&]() -> void {
		ENGINE_LOG_INFO("Starting render thread.");
		while(!windowManager->windowShouldClose()) {
			renderer->drawFrame();	
			auto interpolationRatio = std::min((renderer->m_lastFrameTime / m_lastTickTime), 1.0f);
			renderer->m_interpolationRatio = interpolationRatio;
			//ENGINE_LOG_ERROR("INTERPOLATION RATIO: {}", interpolationRatio);
		}
	});
}

void Ir::application::run() {
	auto clock = std::chrono::high_resolution_clock();
	while(!windowManager->windowShouldClose()) {
		auto start = clock.now();
		onTick(m_lastTickTime);
		m_lastTickTime = std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(clock.now() - start).count();
	}
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
		app.run();
	} catch (std::exception& e) {
		ENGINE_LOG_FATAL("Oh Fiddlesticks! What now?");
		ENGINE_LOG_FATAL_NP("{}", e.what());
	}

#ifdef _MSC_VER
	timeEndPeriod(1);
#endif
	return 0;
}