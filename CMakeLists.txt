cmake_minimum_required(VERSION 3.15)
project(base CXX)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(spdlog REQUIRED)
find_package(tomlplusplus REQUIRED)
find_package(asio REQUIRED)
find_package(nlohmann_json REQUIRED)

add_library(base
    src/logger.cpp
    src/config.cpp
    src/application.cpp
    src/messaging.cpp
    src/cli.cpp
    src/tables.cpp
)
target_include_directories(base PUBLIC include)

if (NOT BUILD_TESTING STREQUAL OFF)
    add_subdirectory(tests)
endif()

if (NOT BUILD_EXAMPLE STREQUAL OFF)
    add_subdirectory(examples)
endif()

if (NOT BUILD_BENCHMARK STREQUAL OFF)
    add_subdirectory(benchmarks)
endif()

target_link_libraries(base
    spdlog::spdlog
    tomlplusplus::tomlplusplus
    asio::asio
    nlohmann_json::nlohmann_json
)

install(TARGETS base)
