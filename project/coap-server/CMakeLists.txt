cmake_minimum_required(VERSION 3.10)
project(coap-server)
file(GLOB_RECURSE SOURCES RELATIVE ${CMAKE_SOURCE_DIR} "./src/*.cpp")
add_executable(coap-server ${SOURCES})
set_property(TARGET coap-server PROPERTY CXX_STANDARD 20)

find_package(Threads)

include_directories(include)
target_link_libraries(coap-server coap-3)
target_link_libraries(coap-server Threads::Threads)
