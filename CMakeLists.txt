cmake_minimum_required(VERSION 3.10)
project(buff_demo)

add_executable(buff_demo main.cpp)

target_link_libraries(buff_demo iio)

