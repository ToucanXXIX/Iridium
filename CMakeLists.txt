cmake_minimum_required(VERSION 3.5)

project(Iridium VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS True)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

#add_compile_options("-stdlib=libc++")
#add_link_options("-stdlib=libc++")

add_subdirectory(./IridiumEngine)
add_subdirectory(./Demo)
