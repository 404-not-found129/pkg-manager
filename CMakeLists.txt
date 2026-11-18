cmake_minimum_required(VERSION 3.20)
project(pkg_manager CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(CURL REQUIRED)

add_library(pkg_manager STATIC library.cpp)
target_include_directories(pkg_manager PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(pkg_manager PUBLIC CURL::libcurl)

add_executable(pkg main.cpp)
target_link_libraries(pkg PRIVATE pkg_manager)
