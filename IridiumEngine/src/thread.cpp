#include "thread.hpp"

static thread_local std::string g_threadName{"none"};

std::string_view Iridium::getThreadName() noexcept {
	return g_threadName;
}

void Iridium::setThreadName(const std::string& threadName) {
	g_threadName = threadName;
}

