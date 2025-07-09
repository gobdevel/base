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
    src/cli.cpp
    src/tables.cpp
)
target_include_directories(base PUBLIC include)

if (BUILD_TESTING AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests")
    add_subdirectory(tests)
endif()

if (BUILD_EXAMPLE AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples")
    add_subdirectory(examples)
endif()

if (BUILD_BENCHMARK AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/benchmarks")
    add_subdirectory(benchmarks)
endif()

# Note: docs directory contains markdown files only, no CMake build needed

target_link_libraries(base
    spdlog::spdlog
    tomlplusplus::tomlplusplus
    asio::asio
    nlohmann_json::nlohmann_json
)

# Install the library and headers
install(TARGETS base
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)
install(DIRECTORY include/ DESTINATION include)
