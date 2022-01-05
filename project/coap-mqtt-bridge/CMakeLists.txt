cmake_minimum_required(VERSION 3.10)
project(coap-mqtt-bridge)
file(GLOB_RECURSE SOURCES RELATIVE ${CMAKE_SOURCE_DIR} "./src/*.cpp")
add_executable(coap-mqtt-bridge ${SOURCES})
set_property(TARGET coap-mqtt-bridge PROPERTY CXX_STANDARD 20)

find_package(Threads)

include_directories(include)
target_link_libraries(coap-mqtt-bridge Threads::Threads)
