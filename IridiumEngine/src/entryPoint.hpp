#pragma once

#include "appinfo.hpp"

namespace Iridium {
	// +fwd
	namespace Renderer {
		class renderer;
	}
	// -fwd

	class application {
	private:
		
	public:
		virtual appinfo& getAppinfo() = 0;

		Renderer::renderer* renderer;
		class input_handler* inputHandler;
		class shader_compiler* shaderCompiler;
		class window_manager* windowManager;

		application(Iridium::appinfo& info);
	};
}
int main(int, char**);