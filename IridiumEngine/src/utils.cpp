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