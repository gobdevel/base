cmake_minimum_required(VERSION 3.15)
project(base CXX)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(spdlog REQUIRED)
find_package(tomlplusplus REQUIRED)
find_package(asio REQUIRED)

# Include Conan dependencies
include(${CMAKE_BINARY_DIR}/generators/conandeps_legacy.cmake)

add_library(base
    src/logger.cpp
    src/config.cpp
    src/application.cpp
    src/messaging.cpp
    src/cli.cpp
    src/tables.cpp
)
target_include_directories(base PUBLIC include)

# Add nlohmann_json include directory (header-only library)
find_path(NLOHMANN_JSON_INCLUDE_DIR
    NAMES nlohmann/json.hpp
    PATHS /Users/gobindp/.conan2/p/*/p/include
    NO_DEFAULT_PATH
)
if(NLOHMANN_JSON_INCLUDE_DIR)
    target_include_directories(base PRIVATE ${NLOHMANN_JSON_INCLUDE_DIR})
else()
    message(FATAL_ERROR "nlohmann/json.hpp not found")
endif()

# Add examples subdirectory only if it exists (not needed for Conan packages)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples/CMakeLists.txt")
    add_subdirectory(examples)
endif()

# Add benchmark executable only if benchmarks directory exists
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/benchmarks")
    add_executable(simple_benchmark benchmarks/simple_benchmark.cpp)
    target_link_libraries(simple_benchmark base)
    set_target_properties(simple_benchmark PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/benchmarks")
endif()

if (NOT BUILD_TESTING STREQUAL OFF)
    add_subdirectory(tests)
endif()

target_link_libraries(base
    spdlog::spdlog
    tomlplusplus::tomlplusplus
    asio::asio
)

set_target_properties(base PROPERTIES PUBLIC_HEADER "include/logger.h;include/config.h;include/singleton.h;include/application.h;include/messaging.h;include/cli.h;include/tables.h")
install(TARGETS base)
