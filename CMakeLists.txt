cmake_minimum_required(VERSION 3.31)
project(buffer_plot)

add_executable(main main.cpp)
target_link_libraries(main iio)