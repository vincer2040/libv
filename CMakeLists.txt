cmake_minimum_required(VERSION 3.20)
project(libv C)

set(CMAKE_C_COMPILER "clang")
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

enable_testing()
add_subdirectory(libv)



