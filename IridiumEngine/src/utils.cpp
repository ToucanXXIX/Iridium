#include "utils.hpp"

#include "entryPoint.hpp"

namespace Iridium {
	namespace Impl { // TODO(): this could probably be wrapped in an atomic or have
	                 // a mutex.
		static application *g_applicationPointer = nullptr;
	}
} // namespace Iridium

Iridium::application* Iridium::getApplicationPointer() {
	return Iridium::Impl::g_applicationPointer;
}

Iridium::application* Iridium::setApplicationPointer(Iridium::application* pointer, Iridium::warning) {
	Iridium::application *old = Iridium::Impl::g_applicationPointer;
	Iridium::Impl::g_applicationPointer = pointer;
	return old;
}

size_t Iridium::unicodeToUTF8Lenght(std::vector<char32_t> unicode) {
	size_t length = 0;
	for(const auto& codepoint : unicode) {
		if(codepoint <= 0x7F) {
			length += 1;
		} else if(codepoint <= 0x7FF) {
			length += 2;
		} else if(codepoint <= 0xFFFF) {
			length += 3;
		} else if(codepoint <= 0x10FFFF) {
			length += 4;
		}
	}
	return length;
}

std::vector<char> Iridium::unicodeToUTF8(std::vector<char32_t> unicode) {
	std::vector<char> utf8Text{};
	for(const auto& codepoint : unicode) {
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
	return utf8Text;
}