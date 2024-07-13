cmake_minimum_required(VERSION 3.22)
project(p1 C)

set(CMAKE_C_STANDARD 11)

file(GLOB src
        "*.h"
        "*.c"
)
add_executable(p1 ${src})
