cmake_minimum_required(VERSION 3.16)
project(example_terminal_server C)

add_subdirectory(../../.. proto)

add_executable(example_terminal_server server.c)

target_include_directories(example_terminal_server PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../../include")
target_link_libraries(example_terminal_server PUBLIC channels_proto channels_proto_server)
