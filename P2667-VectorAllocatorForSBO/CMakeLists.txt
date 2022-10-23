cmake_minimum_required(VERSION 3.14)
project(buffered_allocator)
set(CMAKE_CXX_STANDARD 20)

add_executable(test_vector test_vector.cpp experimental_vector.h experimental_memory.h)

