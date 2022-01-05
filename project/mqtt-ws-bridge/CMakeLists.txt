cmake_minimum_required(VERSION 3.10)
project(mqtt-ws-bridge)
file(GLOB_RECURSE SOURCES RELATIVE ${CMAKE_SOURCE_DIR} "./src/*.cpp")
add_executable(mqtt-ws-bridge ${SOURCES})
set_property(TARGET mqtt-ws-bridge PROPERTY CXX_STANDARD 20)

#Optional:
include_directories(include)

find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
target_link_libraries(mqtt-ws-bridge Threads::Threads)
target_link_libraries(mqtt-ws-bridge OpenSSL::SSL)
