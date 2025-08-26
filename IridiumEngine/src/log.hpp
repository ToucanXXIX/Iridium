#pragma once

#include <cstdint>
#include <format>
#include <print>
#include <string>
#include <utility>

#define ENGINE_DEBUG

#ifdef ENGINE_DEBUG
#define ENGINE_LOG_INFO(fmt, ...)  Iridium::Logger::log(Iridium::Logger::severity::INFO, fmt, ##__VA_ARGS__)
#define ENGINE_LOG_WARN(fmt, ...)  Iridium::Logger::log(Iridium::Logger::severity::WARN, fmt, ##__VA_ARGS__)
#define ENGINE_LOG_ERROR(fmt, ...) Iridium::Logger::log(Iridium::Logger::severity::ERROR,fmt, ##__VA_ARGS__)
#define ENGINE_LOG_FATAL(fmt, ...) Iridium::Logger::log(Iridium::Logger::severity::FATAL,fmt, ##__VA_ARGS__)

#define ENGINE_LOG_INFO_NP(fmt, ...)  Iridium::Logger::logNP(Iridium::Logger::severity::INFO, fmt, ##__VA_ARGS__)
#define ENGINE_LOG_WARN_NP(fmt, ...)  Iridium::Logger::logNP(Iridium::Logger::severity::WARN, fmt, ##__VA_ARGS__)
#define ENGINE_LOG_ERROR_NP(fmt, ...) Iridium::Logger::logNP(Iridium::Logger::severity::ERROR,fmt, ##__VA_ARGS__)
#define ENGINE_LOG_FATAL_NP(fmt, ...) Iridium::Logger::logNP(Iridium::Logger::severity::FATAL,fmt, ##__VA_ARGS__)

#else
#define ENGINE_LOG_INFO(fmt, ...)
#define ENGINE_LOG_WARN(fmt, ...)
#define ENGINE_LOG_ERROR(fmt, ...)
#define ENGINE_LOG_FATAL(fmt, ...)

#define ENGINE_LOG_INFO_NP(fmt, ...) 
#define ENGINE_LOG_WARN_NP(fmt, ...) 
#define ENGINE_LOG_ERROR_NP(fmt, ...)
#define ENGINE_LOG_FATAL_NP(fmt, ...)

#endif

namespace Iridium {
	namespace Logger {
		enum severity : uint8_t {
			INFO = (1 << 0),
			WARN = (1 << 1),
			ERROR= (1 << 2),
			FATAL= (1 << 3)
		};

		enum empty_prefix{};

		std::string getTimestamp();
		std::string getPrefix(severity level);
		std::string getPrefix(severity level, empty_prefix);

		template<typename... Args>
		void log(severity level, std::format_string<Args...> fmt, Args&&... args) {
			std::string prefix = getPrefix(level);
			std::string message = std::format(fmt, std::forward<Args>(args)...);
			std::println("{}: {}" "\033[0m", prefix, message);
		}

		template<typename... Args>
		void logNP(severity level, std::format_string<Args...> fmt, Args&&... args) {
			std::string prefix = getPrefix(level, empty_prefix());
			std::string message = std::format(fmt, std::forward<Args>(args)...);
			std::println("{}| {}" "\033[0m", prefix, message);
		}
	}
}