set(SOURCES
	src/Demo.cpp
)

add_executable(Demo ${SOURCES})
target_include_directories(Demo PRIVATE ${CMAKE_SOURCE_DIR}/IridiumEngine/src/)
target_link_libraries(Demo IridiumEngine)