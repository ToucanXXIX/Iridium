#pragma once

#include <cstdint>

namespace Iridium {
	struct appinfo {
		const char* name;
		struct {
			uint32_t major;
			uint32_t minor;
			uint32_t patch;
		} version;
	};
}