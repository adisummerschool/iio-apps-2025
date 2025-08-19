cmake_minimum_required(VERSION 3.10)
project(LibiioDupe C)

add_library(print SHARED print.c)
target_include_directories(print PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(main main.c)

target_link_libraries(main iio print)
