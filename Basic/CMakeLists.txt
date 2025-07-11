cmake_minimum_required(VERSION 3.15)

set(CMAKE_C_STANDARD 11)

add_executable(netprobe main.c netprobe.c
        cross_platform_socket.h)

if (WIN32)
    target_link_libraries(netprobe wsock32 ws2_32)
endif ()
