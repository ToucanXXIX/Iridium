include(FetchContent)

project(IridiumEngine VERSION 1.0.0 LANGUAGES CXX)

set(ENGINE_RESCOURCES
	src/utils.cpp
	src/utils.hpp
	src/log.cpp
	src/log.hpp
	src/entryPoint.cpp
	src/entryPoint.hpp
	src/thread.cpp
	src/thread.hpp
	src/inputHandler.cpp
	src/inputHandler.hpp
)

set(ENGINE_ASSETS_RESCOURCES
	src/assets/shader.cpp
	src/assets/shader.hpp
	src/assets/shaderCompiler.cpp
	src/assets/shaderCompiler.hpp
)

set(ENGINE_RENDERER_RESCOURCES
	src/renderer/window.cpp
	src/renderer/window.hpp
	src/renderer/vulkan.cpp
	src/renderer/vulkan.hpp
	src/renderer/renderer.cpp
	src/renderer/renderer.hpp
	src/renderer/vertex.cpp
	src/renderer/vertex.hpp
)

# set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

add_library(IridiumEngine ${ENGINE_RESCOURCES} ${ENGINE_RENDERER_RESCOURCES} ${ENGINE_ASSETS_RESCOURCES})

target_compile_definitions(IridiumEngine PUBLIC
	GLM_FORCE_RADIANS
	GLM_FORCE_DEFAULT_ALIGNED_TYPES

	$<$<CONFIG:Debug>:USE_VALIDATION_LAYERS=1>
	$<$<NOT:$<CONFIG:Debug>>:USE_VALIDATION_LAYERS=0>
)

find_package(Vulkan REQUIRED COMPONENTS SPIRV-Tools)
find_package(Python3 REQUIRED)

#add_subdirectory(./dependencies/glfw-3.4)

FetchContent_Declare(
	glslang
	GIT_REPOSITORY https://github.com/KhronosGroup/glslang.git
	GIT_TAG main
)
set(ENABLE_GLSLANG_BINARIES OFF CACHE BOOL "Disable glslang command-line tools")
set(ENABLE_OPT ON CACHE BOOL "enable SPIR-V opt")

FetchContent_MakeAvailable(glslang)

execute_process(
	COMMAND ${Python3_EXECUTABLE} ${glslang_SOURCE_DIR}/update_glslang_sources.py
	WORKING_DIRECTORY ${glslang_SOURCE_DIR}
	RESULT_VARIABLE GLSLANG_SCRIPT_RESULT
    OUTPUT_VARIABLE GLSLANG_SCRIPT_OUTPUT
    ERROR_VARIABLE  GLSLANG_SCRIPT_ERROR
)

if(NOT GLSLANG_SCRIPT_RESULT EQUAL 0)
    message(FATAL_ERROR
        "glslang setup script failed (exit ${GLSLANG_SCRIPT_RESULT}):\n"
        "${GLSLANG_SCRIPT_ERROR}"
    )
else()
    message(STATUS "glslang sources updated successfully")
endif()

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
	glfw
	glm::glm
#Vulkan
	Vulkan::Vulkan
	SPIRV-Tools-opt
	Vulkan::SPIRV-Tools
#glslang
	glslang
	SPIRV
    glslang-default-resource-limits
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	target_compile_options(IridiumEngine PRIVATE "-Wextra" "-Wall" "-Werror" "-Wpedantic" "-Wno-gnu-zero-variadic-macro-arguments")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
endif()