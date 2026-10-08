#pragma once

#include "appinfo.hpp"

namespace Iridium {
	// +fwd
	namespace Renderer {
		class renderer;
	}

	class application {
		friend int main(int argc, char** argv);

	private:
		float m_lastTickTime = 1; //milliseconds

		public:
		virtual appinfo& getAppinfo() = 0;
		virtual void onTick(float dt) = 0;

		Renderer::renderer* renderer;
		class input_handler* inputHandler;
		class shader_compiler* shaderCompiler;
		class window_manager* windowManager;
		class thread_manager* threadManager;

		application(Iridium::appinfo& info);

		void run();
	};
}
int main(int, char**);