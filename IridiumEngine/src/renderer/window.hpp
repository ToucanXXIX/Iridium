#pragma once

namespace Iridium {
	using window_ptr = void*;

	
	class window_manager {
		public:
		window_manager();
		~window_manager();
		
		void createWindow(int width, int height, const char* name = "Iridium");
		void setWindowName(const char* name);
		bool windowShouldClose();
		
		window_ptr getWindowHandle();
		private:
		window_ptr m_window = nullptr;
	};

	window_manager* getWindowManager();
}