cmake_minimum_required(VERSION 3.16)
project(channels_proto C ASM)

add_library(channels_proto OBJECT
    src/proto.c
    src/proto_objects.c
    src/proto_req.c
)

add_library(channels_proto_asm OBJECT
    src/proto_object_read.asm
    src/proto_objects.asm
)

add_library(channels_proto_server OBJECT
    src/proto_heap.c
    src/proto_req_handlers.c
)

target_include_directories(channels_proto PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(channels_proto_server PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
