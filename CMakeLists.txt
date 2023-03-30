cmake_minimum_required(VERSION 3.22)

set(CMAKE_CXX_STANDARD 17)
project(easylog4cpp)


aux_source_directory(src LIB_SRC)
include_directories(src)

add_library(libeasylog4cpp OBJECT ${LIB_SRC})

add_executable(test test/main.cpp)
target_link_libraries(test libeasylog4cpp)