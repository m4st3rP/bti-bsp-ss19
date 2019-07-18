cmake_minimum_required(VERSION 3.14)
project(V1 C)

set(CMAKE_C_STANDARD 11)

include_directories(src)

add_executable(V1
        src/getcharTimeout.c
        src/getcharTimeout.h
        src/main.c
        src/main.h
        src/queue.c
        src/queue.h
        src/taskqueue.c
        src/taskqueue.h)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(V1 Threads::Threads rt)