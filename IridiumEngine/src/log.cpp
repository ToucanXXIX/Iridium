#include "log.hpp"

#include <ctime>
#include <format>
#include <string_view>

#include "thread.hpp"

#define CONSOLE_TEXT_BLACK "\033[30m"
#define CONSOLE_TEXT_RED "\033[31m"
#define CONSOLE_TEXT_GREEN "\033[32m"
#define CONSOLE_TEXT_YELLOW "\033[33m"
#define CONSOLE_TEXT_BLUE "\033[34m"
#define CONSOLE_TEXT_MAGENTA "\033[35m"
#define CONSOLE_TEXT_CYAN "\033[36m"
#define CONSOLE_TEXT_LIGHTGRAY "\033[37m"
#define CONSOLE_TEXT_DARKGRAY "\033[90m"
#define CONSOLE_TEXT_LIGHTRED "\033[91m"
#define CONSOLE_TEXT_LIGHTGREEN "\033[92m"
#define CONSOLE_TEXT_LIGHTYELLOW "\033[93m"
#define CONSOLE_TEXT_LIGHTBLUE "\033[94m"
#define CONSOLE_TEXT_LIGHTMAGENTA "\033[95m"
#define CONSOLE_TEXT_LIGHTCYAN "\033[96m"
#define CONSOLE_TEXT_WHITE "\033[97m"

#define CONSOLE_TEXT_BOLD "\033[1m"
#define CONSOLE_TEXT_NOBOLD "\033[22m"
#define CONSOLE_TEXT_UNDERLINE "\033[4m"
#define CONSOLE_TEXT_NOUNDERLINE "\033[24m"
#define CONSOLE_TEXT_NEGATIVE "\033[7m"
#define CONSOLE_TEXT_POSITIVE "\033[27m"

#define CONSOLE_BACK_BLACK "\033[40m"
#define CONSOLE_BACK_RED "\033[41m"
#define CONSOLE_BACK_GREEN "\033[42m"
#define CONSOLE_BACK_YELLOW "\033[43m"
#define CONSOLE_BACK_BLUE "\033[44m"
#define CONSOLE_BACK_MAGENTA "\033[45m"
#define CONSOLE_BACK_CYAN "\033[46m"
#define CONSOLE_BACK_LIGHTGRAY "\033[47m"
#define CONSOLE_BACK_DARKGRAY "\033[100m"
#define CONSOLE_BACK_LIGHTRED "\033[101m"
#define CONSOLE_BACK_LIGHTGREEN "\033[102m"
#define CONSOLE_BACK_LIGHTYELLOW "\033[103m"
#define CONSOLE_BACK_LIGHTBLUE "\033[104m"
#define CONSOLE_BACK_LIGHTMAGENTA "\033[105m"
#define CONSOLE_BACK_LIGHTCYAN "\033[106m"
#define CONSOLE_BACK_WHITE "\033[107m"

#define CONSOLE_DEFAULT "\033[0m"

std::string Iridium::Logger::getTimestamp() {
	std::time_t time = std::time(nullptr);
	std::tm* tm = std::localtime(&time);
	return std::format("{:0>2}:{:0>2}:{:0>2}", tm->tm_hour, tm->tm_min, tm->tm_sec);
}

std::string Iridium::Logger::getPrefix(severity level) {
	std::string_view levelStr{};
	switch(level) {
		case INFO:
			levelStr = "[Info]";
			break;
		case WARN:
			levelStr = CONSOLE_TEXT_YELLOW "[Warn]";
			break;
		case ERROR:
			levelStr = CONSOLE_TEXT_LIGHTRED "[Error]";
			break;
		case FATAL:
			levelStr = CONSOLE_TEXT_WHITE CONSOLE_BACK_RED "[Fatal]";
			break;
	}
	std::string_view threadName = Iridium::getThreadName();
	std::string prefix = std::format("{}[{}][{}]", levelStr, threadName, getTimestamp());
	return prefix;
}

//TODO: This is bad, redo later :)
std::string Iridium::Logger::getPrefix(severity level, empty_prefix) {
	std::string_view levelStr{};
	switch(level) {
		case INFO:
			levelStr = "      ";
			break;
		case WARN:
			levelStr = CONSOLE_TEXT_YELLOW "      ";
			break;
		case ERROR:
			levelStr = CONSOLE_TEXT_LIGHTRED "       ";
			break;
		case FATAL:
			levelStr = CONSOLE_TEXT_WHITE CONSOLE_BACK_RED "       ";
			break;
	}
	std::string threadName(Iridium::getThreadName().length(), ' ');
	std::string prefix = std::format("{} {}  {} ", levelStr, threadName, std::string(getTimestamp().length(), ' '));
	return prefix;
}