#include "shader.hpp"

#include "glslang/SPIRV/GlslangToSpv.h"
#include "glslang/Public/ResourceLimits.h"
#include "glslang/Public/ShaderLang.h"
#include "spirv-tools/optimizer.hpp"

#include <spirv-tools/libspirv.h>
#include <spirv-tools/libspirv.hpp>
#include <stdexcept>
#include <string>
#include <ranges>

#include "../log.hpp"

Iridium::shader_type::shader_type(Iridium::shader_type::underlying_enum enumerator) {
	value = enumerator;
}

Iridium::shader_type& Iridium::shader_type::operator=(Iridium::shader_type::underlying_enum enumerator) {
	value = enumerator;
	return *this;
}

const char* Iridium::shader_type::str() const {
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
	ENGINE_LOG_INFO("Starting compilation of {} shader. ({})", stage.str(), shaderSources.size());
	
	std::vector<const char*> shaderSourcesRaw(shaderSources.size());
	std::vector<int> shaderSourcesLen(shaderSources.size());
	for(const auto& [index, source] : std::views::enumerate(shaderSources)) {
		shaderSourcesRaw[index] = source.c_str();
		shaderSourcesLen[index] = source.length();
	}

	EShLanguage glslStage = shaderTypeToEShLanguage(stage);
	glslang::EShClient client = glslang::EShClientVulkan;

	spvtools::MessageConsumer messageConsumer([](spv_message_level_t level, [[maybe_unused]] const char* source, [[maybe_unused]] const spv_position_t& pos, const char* msg) -> void {
		switch(level) {
			using enum spv_message_level_t;
			case SPV_MSG_FATAL: 
			case SPV_MSG_INTERNAL_ERROR:
				ENGINE_LOG_FATAL("SPIRV: {}", msg);
				break;
			case SPV_MSG_ERROR:
				ENGINE_LOG_ERROR("SPIRV: {}", msg);
				break;
			case SPV_MSG_WARNING:
				ENGINE_LOG_WARN("SPIRV: {}", msg);
				break;
			case SPV_MSG_INFO:
			case SPV_MSG_DEBUG:
				ENGINE_LOG_INFO("SPIRV: {}", msg);
				break;
		};
	});

	glslang::TShader shader(glslStage);
	shader.setStringsWithLengths(shaderSourcesRaw.data(), shaderSourcesLen.data(), static_cast<int>(shaderSourcesRaw.size()));
	shader.setEnvInput(glslang::EShSourceGlsl, glslStage, client, 450);
	shader.setEnvClient(client, glslang::EShTargetVulkan_1_3);
	shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_0);
	if(!shader.parse(GetDefaultResources(), 450, false, EShMsgDefault))
		throw std::runtime_error(std::string("shader parsing failed: ") + shader.getInfoLog());

	glslang::TProgram program;

	program.addShader(&shader);
	if(!program.link(EShMsgDefault))
		throw std::runtime_error(std::string("shader program linking failed: ") + program.getInfoLog());

	std::vector<uint32_t> spirv{};
	glslang::TIntermediate* intermediate = program.getIntermediate(glslStage);
#ifdef _MSC_VER
	glslang::GlslangToSpv(*intermediate, spirv); //Other version causes a crash on msvc
#else
	glslang::SpvOptions options{
		.generateDebugInfo = true,
		.stripDebugInfo = false,
		.optimizeSize = true,
	};
	spv::SpvBuildLogger logger{};
	glslang::GlslangToSpv(*intermediate, spirv, &logger, &options);
	std::string shaderLogs = logger.getAllMessages();
	if(!shaderLogs.empty())
		ENGINE_LOG_INFO("{}", shaderLogs);
#endif
	ENGINE_LOG_INFO("Finished compiling {} shader.", stage.str());

	std::vector<uint32_t> optimizedSpirv;
	spvtools::Optimizer optimizer(SPV_ENV_VULKAN_1_3);
	optimizer.SetMessageConsumer(messageConsumer);
	optimizer.RegisterPerformancePasses();
	optimizer.Run(spirv.data(), spirv.size(), &optimizedSpirv);

	return optimizedSpirv;
}