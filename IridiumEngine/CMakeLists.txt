include(FetchContent)

project(IridiumEngine VERSION 1.0.0 LANGUAGES CXX)

set(ENGINE_RESCOURCES
	src/log.cpp
	src/entryPoint.cpp
	src/thread.cpp
)

set(ENGINE_RENDERER_RESCOURCES
	src/renderer/window.cpp
	src/renderer/vulkan.cpp
	src/renderer/shader.cpp
)

add_library(IridiumEngine ${ENGINE_RESCOURCES} ${ENGINE_RENDERER_RESCOURCES})

target_compile_definitions(IridiumEngine PUBLIC
	$<$<CONFIG:Debug>:USE_VALIDATION_LAYERS=1>
	$<$<NOT:$<CONFIG:Debug>>:USE_VALIDATION_LAYERS=0>
)

find_package(Vulkan REQUIRED COMPONENTS SPIRV-Tools)

#add_subdirectory(./dependencies/glfw-3.4)

FetchContent_Declare(
	glslang
	GIT_REPOSITORY https://github.com/KhronosGroup/glslang.git
	GIT_TAG main
)
set(ENABLE_GLSLANG_BINARIES OFF CACHE BOOL "Disable glslang command-line tools")
set(ENABLE_SPVREMAPPER OFF CACHE BOOL "Disable SPIR-V remapper")
set(ENABLE_HLSL OFF CACHE BOOL "Disable HLSL support")
set(ENABLE_OPT OFF CACHE BOOL "disable SPIR-V opt")

FetchContent_MakeAvailable(glslang)

FetchContent_Declare(
	glm
	GIT_REPOSITORY https://github.com/g-truc/glm.git
	GIT_TAG 2d4c4b4dd31fde06cfffad7915c2b3006402322f
)
FetchContent_MakeAvailable(glm)

FetchContent_Declare(
	glfw
	GIT_REPOSITORY https://github.com/glfw/glfw.git
	GIT_TAG master
)
FetchContent_MakeAvailable(glfw)

target_link_libraries(IridiumEngine PRIVATE
#Vulkan
	Vulkan::Vulkan
	Vulkan::SPIRV-Tools
#glslang
	glslang
	SPIRV
    glslang-default-resource-limits
	
	glfw
	glm::glm
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	target_compile_options(IridiumEngine PRIVATE "-Wextra" "-Wall" "-Werror" "-Wpedantic" "-Wno-gnu-zero-variadic-macro-arguments")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
endif()