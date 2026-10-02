#pragma once

#include <tuple>
#include <cstdint>
#include <functional>

namespace Iridium {
	using window_ptr = void*;

#if 0
	using resize_callback = void(*)(int width, int height);
	using framebuffer_resize_callback = void(*)(int width, int height);
	using key_callback = void(*)(int key, int scancode, int action, int mods);
	using char_callback = void(*)(unsigned int codepoint);
#else
	using resize_callback = std::function<void(int width, int height)>;
	using framebuffer_resize_callback = std::function<void(int width, int height)>;
	using key_callback = std::function<void(int key, int scancode, int action, int mods)>;
	using char_callback = std::function<void(unsigned int codepoint)>;
#endif

	class window_manager {
		public:
		window_manager();
		~window_manager();
		
		void createWindow(int width, int height, const char* name = "Iridium");
		void setWindowName(const char* name);

		resize_callback setResizeCallback(resize_callback newCallback);
		framebuffer_resize_callback setFramebufferResizeCallback(framebuffer_resize_callback newCalback);
		key_callback setKeyCallback(key_callback newCallback);
		char_callback setCharCallback(char_callback newCallback);

		void pollEvents(); 
		std::tuple<uint32_t, uint32_t> framebufferSize();
		bool windowShouldClose();
		
		window_ptr getWindowHandle();
		
		private:
		bool m_shouldClose = false;
		uint32_t m_windowWidht = 0;
		uint32_t m_windowHeight = 0;
		uint32_t m_framebufferWidth = 0;
		uint32_t m_framebufferHeight = 0;
		window_ptr m_window = nullptr;
	
		struct {
			resize_callback resizeCallback = nullptr;
			framebuffer_resize_callback framebufferCallback = nullptr;
			key_callback keyCallback = nullptr;
			char_callback charCallback = nullptr;
		} m_callbacks;
	};

	window_manager* getWindowManager();
}