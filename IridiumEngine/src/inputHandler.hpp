#pragma once

#include <cstddef>
#include <mutex>
#include <vector>

namespace Iridium {
	enum keyboard_key : unsigned short {
		// printable keys
		KEY_GRAVE,
		KEY_1,
		KEY_2,
		KEY_3,
		KEY_4,
		KEY_5,
		KEY_6,
		KEY_7,
		KEY_8,
		KEY_9,
		KEY_0,
		KEY_MINUS,
		KEY_EQUALS,
		KEY_Q,
		KEY_W,
		KEY_E,
		KEY_R,
		KEY_T,
		KEY_Y,
		KEY_U,
		KEY_I,
		KEY_O,
		KEY_P,
		KEY_LBRACKET,
		KEY_RBRACKET,
		KEY_BACKSLASH,
		KEY_A,
		KEY_S,
		KEY_D,
		KEY_F,
		KEY_G,
		KEY_H,
		KEY_J,
		KEY_K,
		KEY_L,
		KEY_SEMICOLON,
		KEY_APOSTROPHE,
		KEY_Z,
		KEY_X,
		KEY_C,
		KEY_V,
		KEY_B,
		KEY_N,
		KEY_M,
		KEY_COMMA,
		KEY_DOT,
		KEY_SLASH,
		KEY_SPACE,

		// non-printable keys
		// function keys
		KEY_ESC,
		KEY_F1,
		KEY_F2,
		KEY_F3,
		KEY_F4,
		KEY_F5,
		KEY_F6,
		KEY_F7,
		KEY_F8,
		KEY_F9,
		KEY_F10,
		KEY_F11,
		KEY_F12,
		KEY_F13,
		KEY_F14,
		KEY_F15,
		KEY_F16,
		KEY_F17,
		KEY_F18,
		KEY_F19,
		KEY_F20,
		KEY_F21,
		KEY_F22,
		KEY_F23,
		KEY_F24,
		KEY_F25,
		KEY_PRINT_SCREEN,
		KEY_SCROLL_LOCK,
		KEY_PAUSE,
		KEY_INSERT,
		KEY_HOME,
		KEY_PAGE_UP,
		KEY_DELETE,
		KEY_END,
		KEY_PAGE_DOWN,
		KEY_ARROW_UP,
		KEY_ARROW_DOWN,
		KEY_ARROW_LEFT,
		KEY_ARROW_RIGHT,
		KEY_TAB,
		KEY_BACKSPACE,
		KEY_CAPSLOCK,
		KEY_ENTER,
		KEY_LSHIFT,
		KEY_RSHIFT,
		KEY_LCONTROL,
		KEY_RCONTROL,
		KEY_LALT,
		KEY_RALT,
		KEY_LSUPER,
		KEY_RSUPER,
		KEY_MENU,
		KEY_NUMLOCK,

		// numpad keys
		KEY_NUMPAD_SLASH,
		KEY_NUMPAD_ASTERIX,
		KEY_NUMPAD_MINUS,
		KEY_NUMPAD_PLUS,
		KEY_NUMPAD_ENTER,
		KEY_NUMPAD_DOT,
		KEY_NUMPAD_EQUALS,
		KEY_NUMPAD_1,
		KEY_NUMPAD_2,
		KEY_NUMPAD_3,
		KEY_NUMPAD_4,
		KEY_NUMPAD_5,
		KEY_NUMPAD_6,
		KEY_NUMPAD_7,
		KEY_NUMPAD_8,
		KEY_NUMPAD_9,
		KEY_NUMPAD_0,

		KEY_UNKNOWN = (unsigned short)-1,

		KEY_MAX_VALUE = KEY_NUMPAD_0,
	};

	class input_handler {
	public:
		input_handler();
		~input_handler();

		std::vector<char32_t> getTextInputAndClear();
		
		bool isKeyPressed(keyboard_key key);

	private:
		std::mutex m_keyStateMutex;
		bool* m_keyStateArray;

		std::mutex m_textInputMutex;
		size_t m_textInputBufferCursor;
		size_t m_textInputBufferSize;
		char32_t* m_textInputBuffer;
	};

	input_handler* getInputHandler();
}