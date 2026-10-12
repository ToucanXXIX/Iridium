#include "thread.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

static thread_local std::string g_threadName{"none"};

std::string_view Iridium::getThreadName() noexcept {
	return g_threadName;
}

void Iridium::setThreadName(const std::string& threadName) {
	g_threadName = threadName;
#ifdef _WIN32
	std::wstring wideThreadName = std::wstring(threadName.begin(), threadName.end());

	SetThreadDescription(GetCurrentThread(), wideThreadName.c_str());
#endif
}

