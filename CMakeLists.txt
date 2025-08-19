cmake_minimum_required(VERSION 3.31.6)
project(MainProgram C)

add_executable(main main.cpp)

target_link_libraries(main iio)