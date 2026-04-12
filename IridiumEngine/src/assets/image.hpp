#include <cstddef>

#include <GLFW/glfw3.h>

namespace Iridium {
	class image {
	public:
		image();
		
		size_t m_width;
		size_t m_height;
		char* m_data;
	};
}