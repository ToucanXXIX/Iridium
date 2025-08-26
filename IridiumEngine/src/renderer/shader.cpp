#include "shader.hpp"

#include "glslang/SPIRV/GlslangToSpv.h"
#include "glslang/Public/ResourceLimits.h"
#include "glslang/Public/ShaderLang.h"
#include <stdexcept>
#include <string>

Iridium::shader_type::shader_type(Iridium::shader_type::underlying_enum enumerator) {
	value = enumerator;
}

Iridium::shader_type& Iridium::shader_type::operator=(Iridium::shader_type::underlying_enum enumerator) {
	value = enumerator;
	return *this;
}

std::string_view Iridium::shader_type::str() const {
	switch(value) {
		case Iridium::shader_type::none: return "none"; 
		case Iridium::shader_type::vertex: return "vertex";
		case Iridium::shader_type::tesselation: return "tesselation";
		case Iridium::shader_type::geometry: return "geometry";
		case Iridium::shader_type::fragment: return "fragment";
		case Iridium::shader_type::compute: return "compute";
		default: return "none";
	}
}

static EShLanguage shaderTypeToEShLanguage(Iridium::shader_type type) {
	switch(type.value) {
		case Iridium::shader_type::none: return EShLanguage();
		case Iridium::shader_type::vertex: return EShLangVertex;
		case Iridium::shader_type::tesselation: return EShLangTessControl;
		case Iridium::shader_type::geometry: return EShLangGeometry;
		case Iridium::shader_type::fragment: return EShLangFragment;
		case Iridium::shader_type::compute: return EShLangCompute;
	}
	return EShLanguage();
}

Iridium::shader_compiler::shader_compiler() {
	if(!glslang::InitializeProcess())
		throw std::runtime_error("Failed to initialize shader compiler");
}

Iridium::shader_compiler::~shader_compiler() {
	glslang::FinalizeProcess();
}

std::vector<uint32_t> Iridium::shader_compiler::compileShader(const std::vector<std::string>& shaderSources, shader_type stage) {
	std::vector<const char*> shaderSourcesRaw(shaderSources.size());
	std::vector<int> shaderSourcesLen(shaderSources.size());
	for(size_t iterator = 0; iterator < shaderSources.size(); iterator++) {
		shaderSourcesRaw[iterator] = shaderSources[iterator].c_str();
		shaderSourcesLen[iterator] = shaderSources[iterator].length();
	}

	EShLanguage glslStage = shaderTypeToEShLanguage(stage);
	glslang::EShClient client = glslang::EShClientVulkan;

	glslang::TShader shader(glslStage);
	//shader.setStrings(shaderSourcesRaw.data(), static_cast<int>(shaderSourcesRaw.size()));
	shader.setStringsWithLengths(shaderSourcesRaw.data(), shaderSourcesLen.data(), static_cast<int>(shaderSourcesRaw.size()));
	shader.setEnvInput(glslang::EShSourceGlsl, glslStage, client, 450);
	shader.setEnvClient(client, glslang::EShTargetVulkan_1_0);
	shader.setEnvTarget(glslang::EshTargetSpv, glslang::EShTargetSpv_1_0);
	if(!shader.parse(GetDefaultResources(), 450, false, EShMsgDefault))
		throw std::runtime_error(std::string("shader parsing failed: ") + shader.getInfoLog());

	glslang::TProgram program;
	program.addShader(&shader);
	if(!program.link(EShMsgDefault))
		throw std::runtime_error(std::string("shader program linking failed: ") + program.getInfoLog());

	std::vector<uint32_t> spirv{};
	glslang::TIntermediate* intermediate = program.getIntermediate(glslStage);
#ifdef _MSC_VER
	glslang::GlslangToSpv(*intermediate, spirv); //Other version causes a crash on msvc, seems to be an issue with glslang?
#else
	glslang::SpvOptions options{
		.generateDebugInfo = true,
		.stripDebugInfo = false,
		.optimizeSize = true
	};
	spv::SpvBuildLogger logger;
	glslang::GlslangToSpv(*intermediate, spirv, &logger, &options);
#endif // 0


	return spirv;
}