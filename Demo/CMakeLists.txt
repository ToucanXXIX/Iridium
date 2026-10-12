set(SOURCES
	src/Demo.cpp
)

add_executable(Demo ${SOURCES})

target_include_directories(Demo PRIVATE
	${CMAKE_SOURCE_DIR}/IridiumEngine/src/
)

target_link_libraries(Demo
	IridiumEngine
)

add_custom_command(TARGET Demo POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${CMAKE_SOURCE_DIR}/data"
		"$<TARGET_FILE_DIR:Demo>/data"
)