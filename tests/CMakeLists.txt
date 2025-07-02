cmake_minimum_required(VERSION 3.15)
project(CruxTest CXX)

find_package(GTest REQUIRED CONFIG)

add_executable(test_crux
    test_logger.cpp
    test_singleton.cpp
    test_config.cpp
)
target_link_libraries(test_crux GTest::gtest GTest::gtest_main crux)