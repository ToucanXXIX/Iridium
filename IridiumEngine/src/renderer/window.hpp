#pragma once

#include <tuple>
#include <cstdint>

namespace Iridium {
	using window_ptr = void*;

	
	class window_manager {
		public:
		window_manager();
		~window_manager();
		
		void createWindow(int width, int height, const char* name = "Iridium");
		void setWindowName(const char* name);
		
		void pollEvents(); 
		std::tuple<uint32_t, uint32_t> framebufferSize();
		bool windowShouldClose();
		
		window_ptr getWindowHandle();
		private:
		bool m_shouldClose = false;
		uint32_t m_framebufferWidth = 0;
		uint32_t m_framebufferHeight = 0;
		window_ptr m_window = nullptr;
	};

	window_manager* getWindowManager();
}