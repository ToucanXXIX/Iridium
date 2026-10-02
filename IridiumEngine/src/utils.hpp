#pragma once
#include <utility>
#include <vector>

#include "entryPoint.hpp" //TODO(): remove this from here, and add to all places that use getApplicationPointer()

#define CONCAT_IMPL(x, y) x##y
#define CONCAT(x, y) CONCAT_IMPL(x, y)

#define defer(...) auto CONCAT(__defer,__COUNTER__) = Iridium::Impl::makeOnScopeExit([&]() -> void {__VA_ARGS__;})

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
			return on_scope_exit<Callable>{.func = std::move(func)};
		}
	}

	// This denotes functions that will cause irrepairable issues.
	enum warning :unsigned { I_KNOW_WHAT_I_AM_DOING };

	application* getApplicationPointer();
	application* setApplicationPointer(application* pointer, warning);
	
	size_t unicodeToUTF8Lenght(std::vector<char32_t> unicode);
	std::vector<char> unicodeToUTF8(std::vector<char32_t> unicode);
}