cmake_minimum_required(VERSION 3.15)
project(PackageTest CXX)

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(crux CONFIG REQUIRED)

add_executable(example src/example.cpp)
target_link_libraries(example crux::crux)
