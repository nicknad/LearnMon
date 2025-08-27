cmake_minimum_required(VERSION 3.31)
project(LearnMon LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(LearnMon main.cpp)

target_compile_options(LearnMon PRIVATE
        -std=c++23
        -stdlib=libc++
)

target_link_options(LearnMon PRIVATE
        -stdlib=libc++
)