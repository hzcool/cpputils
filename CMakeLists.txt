cmake_minimum_required(VERSION 3.22)
project(cpputils)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-O2 -lpthread")

add_executable(cpputils main.cpp thread_pool.h safe_queue.h timer.h traits.h compile_time_heap_sort.cpp)

add_executable(compile_time_heap_sort compile_time_heap_sort.cpp)