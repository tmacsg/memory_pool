cmake_minimum_required(VERSION 3.5)
project(Memory_Pool LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)

add_executable(test test.cpp allocator.hpp)