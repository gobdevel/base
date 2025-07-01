cmake_minimum_required(VERSION 3.15)
project(crux CXX)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(spdlog REQUIRED)

add_library(crux src/logger.cpp)
target_include_directories(crux PUBLIC include)

if (NOT BUILD_TESTING STREQUAL OFF)
    add_subdirectory(tests)
endif()

target_link_libraries(crux spdlog::spdlog)

set_target_properties(crux PROPERTIES PUBLIC_HEADER "include/logger.h")
install(TARGETS crux)
