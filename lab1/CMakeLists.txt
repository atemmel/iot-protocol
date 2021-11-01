cmake_minimum_required(VERSION 3.10)
project(lab1)
file(GLOB_RECURSE SOURCES RELATIVE ${CMAKE_SOURCE_DIR} "./src/*.cpp")
add_executable(lab1 ${SOURCES})
set_property(TARGET lab1 PROPERTY CXX_STANDARD 17)

include_directories(include)
