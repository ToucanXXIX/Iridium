#pragma once

#include "appinfo.hpp"

namespace Iridium {
	class application {
	private:
		
	public:
		virtual const appinfo& getAppinfo() = 0;
	};
}
int main(int, char**);

int test();