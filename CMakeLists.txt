cmake_minimum_required(VERSION 3.12)
project(hornet_seq CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/O2 /W4)
else()
    add_compile_options(-O3 -Wall -Wextra)
endif()

include_directories(src)

file(GLOB SOURCES src/*.cpp)

add_executable(hornet_seq ${SOURCES})
