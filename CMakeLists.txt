cmake_minimum_required(VERSION 3.8)
project(SIK_zadanie_1)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES server.cpp err.h err.cpp)
#add_executable(err err.cpp err.h)
add_executable(SIK_zadanie_1 ${SOURCE_FILES})