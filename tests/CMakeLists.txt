cmake_minimum_required(VERSION 3.15)
project(CruxTest CXX)

find_package(GTest REQUIRED CONFIG)

add_executable(test_base
    test_logger.cpp
    test_singleton.cpp
    test_config.cpp
    test_application.cpp
    test_messaging.cpp
    test_cli.cpp
)
target_link_libraries(test_base GTest::gtest GTest::gtest_main base)