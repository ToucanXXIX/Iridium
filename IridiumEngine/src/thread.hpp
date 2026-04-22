#pragma once
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
			std::jthread thread([](const char* name, auto func, auto... args) -> void {
				setThreadName(name);
				func(args...);
			}, name, func, args...);
			thread.detach();
		};
	private:
	};
}