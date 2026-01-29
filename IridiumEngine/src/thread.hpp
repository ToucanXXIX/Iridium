#pragma once
#include <string>

namespace Iridium {
	std::string_view getThreadName() noexcept;
	void setThreadName(const std::string& threadName);

	class thread_manager {
	public:

	private:
	};
}