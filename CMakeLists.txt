cmake_minimum_required(VERSION 3.19.0)
project(newton_fractal VERSION 0.0.1)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
enable_language(ISPC)

include(cmake/CPM.cmake)

CPMAddPackage(
	NAME raylib
	GIT_REPOSITORY https://github.com/raysan5/raylib.git
	GIT_TAG 5.5
)

add_executable(newton
	main.cpp
	newton.ispc
)

target_include_directories(newton PUBLIC $<TARGET_PROPERTY:ISPC_HEADER_DIRECTORY>)
target_link_libraries(newton PRIVATE raylib)
set_property(TARGET newton PROPERTY CXX_STANDARD 23)
