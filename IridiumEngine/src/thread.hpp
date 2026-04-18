#pragma once
#include "log.hpp"
#include <exception>
#include <string>
#include <thread>

namespace Iridium {
	std::string_view getThreadName() noexcept;
	void setThreadName(const std::string& threadName);

	class thread_manager {
	public:
		thread_manager() = default;

		template<typename Callable, typename... Args>
		void spawnThread([[maybe_unused]] const char* name, Callable func, Args... args) {
			try {
				std::jthread thread([&]() -> void {
					setThreadName(name);
					func(args...);
				});
				thread.detach();
			} catch(std::exception& e) {
				ENGINE_LOG_ERROR("{}", e.what());
			}
		};
	private:
	};
}