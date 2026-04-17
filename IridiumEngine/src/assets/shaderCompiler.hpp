#include "shader.hpp"

#include <vector>

namespace Iridium {
	class shader_compiler {
	public:
		shader_compiler();
		~shader_compiler();

		std::vector<uint32_t> compileShaderFromFile(const std::vector<const char*> sources, shader_type types);
	private:

	};
}