#include "entryPoint.hpp"
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
#endif

namespace Ir = Iridium;

extern Ir::application& createApp();

Ir::application::application(Iridium::appinfo& info) {
	setApplicationPointer(this, I_KNOW_WHAT_I_AM_DOING);

	windowManager = ::new window_manager();
	windowManager->createWindow(800, 600, info.name);
	inputHandler = ::new input_handler();
	shaderCompiler = ::new shader_compiler();
	renderer = new Renderer::renderer(info);

	renderer->testLoop();
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

	[[maybe_unused]] Iridium::application& app = createApp();
	return 0;
}

int test() {
	return 42;
}