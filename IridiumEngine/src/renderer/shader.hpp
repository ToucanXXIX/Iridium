#pragma once

#include <string>
#include <cstdint>
#include <vector>

namespace Iridium {
	class shader_type {
	public:
		enum underlying_enum {
			none = 0,
			vertex,
			tesselation,
			geometry,
			fragment,
			compute
		};

		underlying_enum value = none;

		shader_type() = default;
		shader_type(underlying_enum enumerator);

		shader_type& operator=(underlying_enum enumerator);

		const char* str() const;
	};


	class shader_compiler {
	private:
	public:
		shader_compiler();
		~shader_compiler();

		[[nodiscard]] std::vector<uint32_t> compileShader(const std::vector<std::string>& shaderSources, shader_type stage);
	};

}