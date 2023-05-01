cmake_minimum_required(VERSION 3.25)

set(CMAKE_USER_MAKE_RULES_OVERRIDE "CMakeFlags.cmake")

project(CallMe)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

enable_testing()
add_subdirectory(UnitTests)
add_subdirectory(pretty)
add_subdirectory(Benchmark)
