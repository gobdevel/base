cmake_minimum_required(VERSION 3.15)
project(crux CXX)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(spdlog REQUIRED)
find_package(tomlplusplus REQUIRED)
find_package(asio REQUIRED)

add_library(crux
    src/logger.cpp
    src/config.cpp
    src/application.cpp
    src/messaging.cpp
    src/cli.cpp
)
target_include_directories(crux PUBLIC include)

# Add examples subdirectory
add_subdirectory(examples)

# Add benchmark executable
add_executable(simple_benchmark benchmarks/simple_benchmark.cpp)
target_link_libraries(simple_benchmark crux)
set_target_properties(simple_benchmark PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/benchmarks")

if (NOT BUILD_TESTING STREQUAL OFF)
    add_subdirectory(tests)
endif()

target_link_libraries(crux
    spdlog::spdlog
    tomlplusplus::tomlplusplus
    asio::asio
)

set_target_properties(crux PROPERTIES PUBLIC_HEADER "include/logger.h;include/config.h;include/singleton.h;include/application.h;include/messaging.h;include/cli.h")
install(TARGETS crux)
