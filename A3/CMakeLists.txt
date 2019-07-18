cmake_minimum_required(VERSION 3.14)
project(A3 C)

set(CMAKE_C_STANDARD 11)

include_directories(src)

add_executable(A3
        src/debug.h
        src/logger.c
        src/logger.h
        src/mmanage.c
        src/mmanage.h
        src/pagefile.c
        src/pagefile.h
        src/syncdataexchange.c
        src/syncdataexchange.h
        src/vmaccess.c
        src/vmaccess.h
        src/vmappl.c
        src/vmappl.h
        src/vmem.h)
