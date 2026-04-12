#pragma once

#include "entryPoint.hpp" //TODO(): remove this from here, and add to all places that use getApplicationPointer()

#define CONCAT_IMPL(x, y) x##y
#define CONCAT(x, y) CONCAT_IMPL(x, y)

#define defer(STATEMENT) auto CONCAT(__defer,__COUNTER__) = Iridium::Impl::makeOnScopeExit([&]() -> void {STATEMENT;})

namespace Iridium {
	namespace Impl {
		template<typename Callable>
		struct on_scope_exit{
			Callable func;
			
			~on_scope_exit() {
				func();
			}
		};
		
		template<typename Callable>
		on_scope_exit<Callable> makeOnScopeExit(Callable func) {
			return on_scope_exit<Callable>{.func = func};
		}
	}

	// This denotes functions that will cause irrepairable issues.
	enum warning :unsigned { I_KNOW_WHAT_I_AM_DOING };

	application* getApplicationPointer();
	application* setApplicationPointer(application* pointer, warning);
}