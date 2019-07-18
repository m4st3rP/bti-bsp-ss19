cmake_minimum_required(VERSION 3.14)
project(A2V1 C)

set(CMAKE_C_STANDARD 11)

include_directories(src)

add_executable(A2V1
        src/main.c
        src/main.h
        src/queue.c
        src/queue.h
        src/definitions.h)
