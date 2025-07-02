cmake_minimum_required(VERSION 3.15)
project(crux CXX)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(spdlog REQUIRED)
find_package(tomlplusplus REQUIRED)

add_library(crux
    src/logger.cpp
    src/config.cpp
)
target_include_directories(crux PUBLIC include)

# Config example executable
add_executable(config_example src/config_example.cpp)
target_link_libraries(config_example crux)

# Color test executable
add_executable(color_test src/color_test.cpp)
target_link_libraries(color_test crux)

# Logger configuration demo executable
add_executable(logger_config_demo src/logger_config_demo.cpp)
target_link_libraries(logger_config_demo crux)

if (NOT BUILD_TESTING STREQUAL OFF)
    add_subdirectory(tests)
endif()

target_link_libraries(crux
    spdlog::spdlog
    tomlplusplus::tomlplusplus
)

set_target_properties(crux PROPERTIES PUBLIC_HEADER "include/logger.h;include/config.h;include/singleton.h")
install(TARGETS crux)
