cmake_minimum_required(VERSION 4.4)
project(pkg_manager)

set(CMAKE_CXX_STANDARD 20)

add_library(pkg_manager STATIC library.cpp)
