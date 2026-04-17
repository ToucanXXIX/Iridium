#include "inputHandler.hpp"
#include "log.hpp"

#include <GLFW//glfw3.h>
#include <cstdio>
#include <cstring>
#include <mutex>

#include "renderer/window.hpp"
#include "renderer/renderer.hpp"
#include "utils.hpp"

static Iridium::keyboard_key glfwKeyToIrKey(int key) {
	switch(key) {
		case GLFW_KEY_SPACE: return Iridium::keyboard_key::KEY_SPACE;
		case GLFW_KEY_APOSTROPHE: return Iridium::keyboard_key::KEY_APOSTROPHE;
		case GLFW_KEY_COMMA: return Iridium::keyboard_key::KEY_COMMA;
		case GLFW_KEY_MINUS: return Iridium::keyboard_key::KEY_MINUS;
		case GLFW_KEY_PERIOD: return Iridium::keyboard_key::KEY_DOT;
		case GLFW_KEY_SLASH: return Iridium::keyboard_key::KEY_SLASH;
		case GLFW_KEY_0: return Iridium::keyboard_key::KEY_0;
		case GLFW_KEY_1: return Iridium::keyboard_key::KEY_1;
		case GLFW_KEY_2: return Iridium::keyboard_key::KEY_2;
		case GLFW_KEY_3: return Iridium::keyboard_key::KEY_3;
		case GLFW_KEY_4: return Iridium::keyboard_key::KEY_4;
		case GLFW_KEY_5: return Iridium::keyboard_key::KEY_5;
		case GLFW_KEY_6: return Iridium::keyboard_key::KEY_6;
		case GLFW_KEY_7: return Iridium::keyboard_key::KEY_7;
		case GLFW_KEY_8: return Iridium::keyboard_key::KEY_8;
		case GLFW_KEY_9: return Iridium::keyboard_key::KEY_9;
		case GLFW_KEY_SEMICOLON: return Iridium::keyboard_key::KEY_SEMICOLON;
		case GLFW_KEY_EQUAL: return Iridium::keyboard_key::KEY_EQUALS;
		case GLFW_KEY_A: return Iridium::keyboard_key::KEY_A;
		case GLFW_KEY_B: return Iridium::keyboard_key::KEY_B;
		case GLFW_KEY_C: return Iridium::keyboard_key::KEY_C;
		case GLFW_KEY_D: return Iridium::keyboard_key::KEY_D;
		case GLFW_KEY_E: return Iridium::keyboard_key::KEY_E;
		case GLFW_KEY_F: return Iridium::keyboard_key::KEY_F;
		case GLFW_KEY_G: return Iridium::keyboard_key::KEY_G;
		case GLFW_KEY_H: return Iridium::keyboard_key::KEY_H;
		case GLFW_KEY_I: return Iridium::keyboard_key::KEY_I;
		case GLFW_KEY_J: return Iridium::keyboard_key::KEY_J;
		case GLFW_KEY_K: return Iridium::keyboard_key::KEY_K;
		case GLFW_KEY_L: return Iridium::keyboard_key::KEY_L;
		case GLFW_KEY_M: return Iridium::keyboard_key::KEY_M;
		case GLFW_KEY_N: return Iridium::keyboard_key::KEY_N;
		case GLFW_KEY_O: return Iridium::keyboard_key::KEY_O;
		case GLFW_KEY_P: return Iridium::keyboard_key::KEY_P;
		case GLFW_KEY_Q: return Iridium::keyboard_key::KEY_Q;
		case GLFW_KEY_R: return Iridium::keyboard_key::KEY_R;
		case GLFW_KEY_S: return Iridium::keyboard_key::KEY_S;
		case GLFW_KEY_T: return Iridium::keyboard_key::KEY_T;
		case GLFW_KEY_U: return Iridium::keyboard_key::KEY_U;
		case GLFW_KEY_V: return Iridium::keyboard_key::KEY_V;
		case GLFW_KEY_W: return Iridium::keyboard_key::KEY_W;
		case GLFW_KEY_X: return Iridium::keyboard_key::KEY_X;
		case GLFW_KEY_Y: return Iridium::keyboard_key::KEY_Y;
		case GLFW_KEY_Z: return Iridium::keyboard_key::KEY_Z;
		case GLFW_KEY_LEFT_BRACKET: return Iridium::keyboard_key::KEY_LBRACKET;
		case GLFW_KEY_BACKSLASH: return Iridium::keyboard_key::KEY_BACKSLASH;
		case GLFW_KEY_RIGHT_BRACKET: return Iridium::keyboard_key::KEY_RBRACKET;
		case GLFW_KEY_GRAVE_ACCENT: return Iridium::keyboard_key::KEY_APOSTROPHE;

		case GLFW_KEY_ESCAPE: return Iridium::keyboard_key::KEY_ESC;
		case GLFW_KEY_ENTER: return Iridium::keyboard_key::KEY_ENTER;
		case GLFW_KEY_TAB: return Iridium::keyboard_key::KEY_TAB;
		case GLFW_KEY_BACKSPACE: return Iridium::keyboard_key::KEY_BACKSPACE;
		case GLFW_KEY_INSERT: return Iridium::keyboard_key::KEY_INSERT;
		case GLFW_KEY_DELETE: return Iridium::keyboard_key::KEY_DELETE;
		case GLFW_KEY_RIGHT: return Iridium::keyboard_key::KEY_ARROW_RIGHT;
		case GLFW_KEY_LEFT: return Iridium::keyboard_key::KEY_ARROW_LEFT;
		case GLFW_KEY_DOWN: return Iridium::keyboard_key::KEY_ARROW_DOWN;
		case GLFW_KEY_UP: return Iridium::keyboard_key::KEY_ARROW_UP;
		case GLFW_KEY_PAGE_UP: return Iridium::keyboard_key::KEY_PAGE_UP;
		case GLFW_KEY_PAGE_DOWN: return Iridium::keyboard_key::KEY_PAGE_DOWN;
		case GLFW_KEY_HOME: return Iridium::keyboard_key::KEY_HOME;
		case GLFW_KEY_END: return Iridium::keyboard_key::KEY_END;
		case GLFW_KEY_CAPS_LOCK: return Iridium::keyboard_key::KEY_CAPSLOCK;
		case GLFW_KEY_SCROLL_LOCK: return Iridium::keyboard_key::KEY_SCROLL_LOCK;
		case GLFW_KEY_NUM_LOCK: return Iridium::keyboard_key::KEY_NUMLOCK;
		case GLFW_KEY_PRINT_SCREEN: return Iridium::keyboard_key::KEY_PRINT_SCREEN;
		case GLFW_KEY_PAUSE: return Iridium::keyboard_key::KEY_PAUSE;
		case GLFW_KEY_F1: return Iridium::keyboard_key::KEY_F1;
		case GLFW_KEY_F2: return Iridium::keyboard_key::KEY_F2;
		case GLFW_KEY_F3: return Iridium::keyboard_key::KEY_F3;
		case GLFW_KEY_F4: return Iridium::keyboard_key::KEY_F4;
		case GLFW_KEY_F5: return Iridium::keyboard_key::KEY_F5;
		case GLFW_KEY_F6: return Iridium::keyboard_key::KEY_F6;
		case GLFW_KEY_F7: return Iridium::keyboard_key::KEY_F7;
		case GLFW_KEY_F8: return Iridium::keyboard_key::KEY_F8;
		case GLFW_KEY_F9: return Iridium::keyboard_key::KEY_F9;
		case GLFW_KEY_F10: return Iridium::keyboard_key::KEY_F10;
		case GLFW_KEY_F11: return Iridium::keyboard_key::KEY_F11;
		case GLFW_KEY_F12: return Iridium::keyboard_key::KEY_F12;
		case GLFW_KEY_F13: return Iridium::keyboard_key::KEY_F13;
		case GLFW_KEY_F14: return Iridium::keyboard_key::KEY_F14;
		case GLFW_KEY_F15: return Iridium::keyboard_key::KEY_F15;
		case GLFW_KEY_F16: return Iridium::keyboard_key::KEY_F16;
		case GLFW_KEY_F17: return Iridium::keyboard_key::KEY_F17;
		case GLFW_KEY_F18: return Iridium::keyboard_key::KEY_F18;
		case GLFW_KEY_F19: return Iridium::keyboard_key::KEY_F19;
		case GLFW_KEY_F20: return Iridium::keyboard_key::KEY_F20;
		case GLFW_KEY_F21: return Iridium::keyboard_key::KEY_F21;
		case GLFW_KEY_F22: return Iridium::keyboard_key::KEY_F22;
		case GLFW_KEY_F23: return Iridium::keyboard_key::KEY_F23;
		case GLFW_KEY_F24: return Iridium::keyboard_key::KEY_F24;
		case GLFW_KEY_F25: return Iridium::keyboard_key::KEY_F25;
		case GLFW_KEY_KP_0: return Iridium::keyboard_key::KEY_NUMPAD_0;
		case GLFW_KEY_KP_1: return Iridium::keyboard_key::KEY_NUMPAD_1;
		case GLFW_KEY_KP_2: return Iridium::keyboard_key::KEY_NUMPAD_2;
		case GLFW_KEY_KP_3: return Iridium::keyboard_key::KEY_NUMPAD_3;
		case GLFW_KEY_KP_4: return Iridium::keyboard_key::KEY_NUMPAD_4;
		case GLFW_KEY_KP_5: return Iridium::keyboard_key::KEY_NUMPAD_5;
		case GLFW_KEY_KP_6: return Iridium::keyboard_key::KEY_NUMPAD_6;
		case GLFW_KEY_KP_7: return Iridium::keyboard_key::KEY_NUMPAD_7;
		case GLFW_KEY_KP_8: return Iridium::keyboard_key::KEY_NUMPAD_8;
		case GLFW_KEY_KP_9: return Iridium::keyboard_key::KEY_NUMPAD_9;
		case GLFW_KEY_KP_DECIMAL: return Iridium::keyboard_key::KEY_NUMPAD_DOT;
		case GLFW_KEY_KP_DIVIDE: return Iridium::keyboard_key::KEY_NUMPAD_SLASH;
		case GLFW_KEY_KP_MULTIPLY: return Iridium::keyboard_key::KEY_NUMPAD_ASTERIX;
		case GLFW_KEY_KP_SUBTRACT: return Iridium::keyboard_key::KEY_NUMPAD_MINUS;
		case GLFW_KEY_KP_ADD: return Iridium::keyboard_key::KEY_NUMPAD_PLUS;
		case GLFW_KEY_KP_ENTER: return Iridium::keyboard_key::KEY_NUMPAD_ENTER;
		case GLFW_KEY_KP_EQUAL: return Iridium::keyboard_key::KEY_NUMPAD_EQUALS;
		case GLFW_KEY_LEFT_SHIFT: return Iridium::keyboard_key::KEY_LSHIFT;
		case GLFW_KEY_LEFT_CONTROL: return Iridium::keyboard_key::KEY_LCONTROL;
		case GLFW_KEY_LEFT_ALT: return Iridium::keyboard_key::KEY_LALT;
		case GLFW_KEY_LEFT_SUPER: return Iridium::keyboard_key::KEY_LSUPER;
		case GLFW_KEY_RIGHT_SHIFT: return Iridium::keyboard_key::KEY_RSHIFT;
		case GLFW_KEY_RIGHT_CONTROL: return Iridium::keyboard_key::KEY_RCONTROL;
		case GLFW_KEY_RIGHT_ALT: return Iridium::keyboard_key::KEY_RALT;
		case GLFW_KEY_RIGHT_SUPER: return Iridium::keyboard_key::KEY_RSUPER;
		case GLFW_KEY_MENU: return Iridium::keyboard_key::KEY_MENU;

		default: return Iridium::KEY_UNKNOWN;
	}
}

Iridium::input_handler::input_handler() {
	window_manager* windowManager = getWindowManager();
	GLFWwindow* window = (GLFWwindow*)windowManager->getWindowHandle();

	m_textInputBufferCursor = 0;
	m_textInputBufferSize = 1024;
	m_textInputBuffer = ::new char32_t[m_textInputBufferSize];
	memset(m_textInputBuffer, 0, m_textInputBufferSize * sizeof(char32_t));

	m_keyStateArray = ::new bool[KEY_MAX_VALUE];
	memset(m_keyStateArray, 0, KEY_MAX_VALUE * sizeof(bool));

	glfwSetKeyCallback(window, [](GLFWwindow*, int key, [[maybe_unused]] int scancode, int action, [[maybe_unused]] int mods) -> void {
		if(action == GLFW_PRESS) {
			std::scoped_lock<std::mutex> lock(getInputHandler()->m_keyStateMutex);
			getInputHandler()->m_keyStateArray[glfwKeyToIrKey(key)] = true;
		}
		else if(action == GLFW_RELEASE) {
			std::scoped_lock<std::mutex> lock(getInputHandler()->m_keyStateMutex);
			getInputHandler()->m_keyStateArray[glfwKeyToIrKey(key)] = false;
		}

		if(key == GLFW_KEY_R && action == GLFW_PRESS)
			Renderer::getRenderer()->drawWireframe = !Renderer::getRenderer()->drawWireframe;

		else if(key == GLFW_KEY_ENTER && action == GLFW_PRESS) {
			auto unicodeText = getInputHandler()->getTextInputAndClear();
			std::vector<char> utf8Text{};
			for(const auto& codepoint : unicodeText) {
				if(codepoint <= 0x7F) {
					utf8Text.push_back(static_cast<char>(codepoint));
				} else if(codepoint <= 0x7FF) {
					utf8Text.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
					utf8Text.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
				} else if(codepoint <= 0xFFFF) {
					utf8Text.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
					utf8Text.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
					utf8Text.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
				} else if(codepoint <= 0x10FFFF) {
					utf8Text.push_back(static_cast<char>(0xF0 | (codepoint >> 18)));
					utf8Text.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
					utf8Text.push_back(static_cast<char>(0x80 | ((codepoint >>  6) & 0x3F)));
					utf8Text.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
				}
			}
			utf8Text.push_back('\0');
			ENGINE_LOG_ERROR("{}", utf8Text.data());
		}
	});

	
	glfwSetCharCallback(window, [](GLFWwindow*, unsigned int codepoint) -> void {
		input_handler* inputHandler = getInputHandler();
		
		// std::vector<char8_t> chars{};
		// if (codepoint <= 0x7F) {
    	// 	chars.push_back(static_cast<char8_t>(codepoint));
    	// } else if (codepoint <= 0x7FF) {
    	// 	chars.push_back(static_cast<char8_t>(0xC0 | (codepoint >> 6)));
    	// 	chars.push_back(static_cast<char8_t>(0x80 | (codepoint & 0x3F)));
    	// } else if (codepoint <= 0xFFFF) {
    	// 	chars.push_back(static_cast<char8_t>(0xE0 | (codepoint >> 12)));
    	// 	chars.push_back(static_cast<char8_t>(0x80 | ((codepoint >> 6) & 0x3F)));
    	// 	chars.push_back(static_cast<char8_t>(0x80 | (codepoint & 0x3F)));
    	// } else if (codepoint <= 0x10FFFF) {
    	// 	chars.push_back(static_cast<char8_t>(0xF0 | (codepoint >> 18)));
    	// 	chars.push_back(static_cast<char8_t>(0x80 | ((codepoint >> 12) & 0x3F)));
    	// 	chars.push_back(static_cast<char8_t>(0x80 | ((codepoint >>  6) & 0x3F)));
    	// 	chars.push_back(static_cast<char8_t>(0x80 | (codepoint & 0x3F)));
    	// }

		{
			std::scoped_lock<std::mutex> lock(inputHandler->m_textInputMutex);

			if(inputHandler->m_textInputBufferCursor + 1 > inputHandler->m_textInputBufferSize) {
				ENGINE_LOG_WARN("textInputBufferCursor has reached {}, did you forget to clear the buffer regularly?", inputHandler->m_textInputBufferCursor);
				return;
			}

			inputHandler->m_textInputBuffer[inputHandler->m_textInputBufferCursor] = codepoint;
			inputHandler->m_textInputBufferCursor += 1;
		}
	});	
}

Iridium::input_handler::~input_handler() {
	::delete[] m_textInputBuffer;
	::delete[] m_keyStateArray;
}

std::vector<char32_t> Iridium::input_handler::getTextInputAndClear() {
	std::vector<char32_t> text{};
	{
		std::scoped_lock<std::mutex> lock(m_textInputMutex);

		text.resize(m_textInputBufferCursor);
		std::memcpy(text.data(), m_textInputBuffer, m_textInputBufferCursor * sizeof(char32_t));
		memset(m_textInputBuffer, '\0', m_textInputBufferSize);
		m_textInputBufferCursor = 0;
	}
	return text;
}

bool Iridium::input_handler::isKeyPressed(Iridium::keyboard_key key) {
	if(key > KEY_MAX_VALUE || key < 0)
		return false;
	return m_keyStateArray[key];
}

Iridium::input_handler* Iridium::getInputHandler() {
	return Iridium::getApplicationPointer()->inputHandler;
}