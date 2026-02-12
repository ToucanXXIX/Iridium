#pragma once

#define CONCAT_IMPL(x, y) x##y
#define CONCAT(x, y) CONCAT_IMPL(x, y)

#define defer(STATEMENT) auto CONCAT(__defer,__COUNTER__) = Iridium::impl::makeOnScopeExit([&]() -> void {STATEMENT})

namespace Iridium {
	namespace impl {
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
}