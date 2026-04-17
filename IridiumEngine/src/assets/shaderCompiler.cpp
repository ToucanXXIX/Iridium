#include "shaderCompiler.hpp"

#include <spirv-tools/libspirv.h>
#include <spirv-tools/libspirv.hpp>
#include <spirv-tools/optimizer.hpp>
#include <glslang/Public/ShaderLang.h>
#include <glslang/MachineIndependent/localintermediate.h>
#include <glslang/Public/ResourceLimits.h>
#include <glslang/SPIRV/GlslangToSpv.h>
#include <fstream>
#include <stdexcept>
#include <ranges>
#include <filesystem>

#include "../log.hpp"

static inline EShLanguage shaderTypeToEShLanguage(Iridium::shader_type type) {
	switch(type) {
		case Iridium::shader_type::none: return EShLanguage();
		case Iridium::shader_type::vertex: return EShLangVertex;
		case Iridium::shader_type::tesselation: return EShLangTessControl;
		case Iridium::shader_type::geometry: return EShLangGeometry;
		case Iridium::shader_type::fragment: return EShLangFragment;
		case Iridium::shader_type::compute: return EShLangCompute;
	}
	return EShLanguage();
}

static void spvToolsMessageConsumer(spv_message_level_t level, [[maybe_unused]] const char* source, [[maybe_unused]] const spv_position_t& pos, const char* msg) {switch(level) {
			using enum spv_message_level_t;
			case SPV_MSG_FATAL: 
			case SPV_MSG_INTERNAL_ERROR:
				ENGINE_LOG_FATAL("SPIRV: {}: {}", source, msg);
				break;
			case SPV_MSG_ERROR:
				ENGINE_LOG_ERROR("SPIRV: {}: {}", source, msg);
				break;
			case SPV_MSG_WARNING:
				ENGINE_LOG_WARN("SPIRV: {}: {}", source, msg);
				break;
			case SPV_MSG_INFO:
			case SPV_MSG_DEBUG:
				ENGINE_LOG_INFO("SPIRV: {}: {}", source, msg);
				break;
		};
}

Iridium::shader_compiler::shader_compiler() {
	if(!glslang::InitializeProcess())
		throw std::runtime_error("Failed to initialize shader compiler.");
}

Iridium::shader_compiler::~shader_compiler() {
	glslang::FinalizeProcess();
}

std::vector<uint32_t> Iridium::shader_compiler::compileShaderFromFile(const std::vector<const char*> filePaths, shader_type type) {
	std::vector<std::string> sources(filePaths.size());
	std::vector<const char*> rawSources(filePaths.size());
	std::vector<int> sourceSizes(filePaths.size());
	for(auto [filePath, source, rawSource, size] : std::views::zip(filePaths, sources, rawSources, sourceSizes)) {
		size_t fileSize = std::filesystem::file_size(filePath);
		std::ifstream file(filePath);
		source.resize(fileSize);
		file.read(source.data(), fileSize);
		rawSource = source.data();
		size = fileSize;
		file.close();
		ENGINE_LOG_INFO("Loaded shader source file {}, with size {}", filePath, size);
	}

	EShLanguage EsType = shaderTypeToEShLanguage(type);

	glslang::EShClient client = glslang::EShClientVulkan;
	spvtools::MessageConsumer messageConsumer(spvToolsMessageConsumer);

	glslang::TShader shader(EsType);
	shader.setStringsWithLengthsAndNames(rawSources.data(), sourceSizes.data(), filePaths.data(), rawSources.size());
	shader.setEnvInput(glslang::EShSourceGlsl, EsType, client, 450);
	shader.setEnvClient(client, glslang::EShTargetVulkan_1_3);
	shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_0);
	if(!shader.parse(GetDefaultResources(), 450, false, EShMsgDefault)) {
		throw std::runtime_error(std::string("shader parsing failed: ") + shader.getInfoLog());
	}

	glslang::TProgram program;
	program.addShader(&shader);
	if(!program.link(EShMsgDefault)) {
		throw std::runtime_error(std::string("shader program linking failed: ") + program.getInfoLog());
	}

	std::vector<uint32_t> spirv{};
	glslang::TIntermediate* intermediate = program.getIntermediate(EsType);

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
	ENGINE_LOG_INFO("Finished compiling shader.");

	std::vector<uint32_t> optimizedSpirv{};
	spvtools::Optimizer optimizer(SPV_ENV_VULKAN_1_3);
	optimizer.SetMessageConsumer(messageConsumer);
	optimizer.RegisterPerformancePasses();
	optimizer.Run(spirv.data(), spirv.size(), &optimizedSpirv);

	ENGINE_LOG_INFO("Generated optimized SPIR-V ({})", optimizedSpirv.size());

	return optimizedSpirv;
}