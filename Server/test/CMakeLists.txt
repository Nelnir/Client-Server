cmake_minimum_required(VERSION 3.10.0)
project(tests)

set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-std=gnu++0x -MD")

add_subdirectory(googletest-master)

enable_testing()

include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})
include_directories(${gmock_SOURCE_DIR}/include ${gmock_SOURCE_DIR})

set(SOURCE_FILES
        main.cpp
        tst_casename.h)

add_executable(tests_run ${SOURCE_FILES})

target_link_libraries(tests_run gtest gtest_main)

add_test(eq tests_run)
