cmake_minimum_required(VERSION 3.10)
project(buff_demo)

include(FetchContent)
FetchContent_Declare(
    matplotlib_cpp
    GIT_REPOSITORY https://github.com/lava/matplotlib-cpp.git
    GIT_TAG master
)

FetchContent_Populate(matplotlib_cpp)

find_package(Python3 REQUIRED COMPONENTS Interpreter Development.Embed)
add_library(matplotlib_cpp INTERFACE)
target_include_directories(matplotlib_cpp SYSTEM 
                           INTERFACE ${matplotlib_cpp_SOURCE_DIR})
target_link_libraries(matplotlib_cpp INTERFACE Python3::Python)

add_executable(buff_demo main.cpp)
target_link_libraries(buff_demo iio matplotlib_cpp)

target_compile_definitions(buff_demo PRIVATE WITHOUT_NUMPY)
