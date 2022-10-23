cmake_minimum_required(VERSION 3.16)
project(example_terminal_client C)

add_subdirectory(../../.. proto)

link_directories("/usr/local/share/z88dk/lib/clibs")
include_directories("/usr/local/share/z88dk/include")

add_executable(example_terminal_client client.c)
target_link_options(example_terminal_client PUBLIC -create-app -lndos)

target_include_directories(example_terminal_client PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../../include")
target_link_libraries(example_terminal_client PUBLIC channels_proto libspectranet.lib libsocket.lib)
