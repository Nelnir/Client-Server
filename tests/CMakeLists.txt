cmake_minimum_required(VERSION 3.10.0)
project(tests)

set(EXE_NAME uTests)

set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-std=gnu++0x -MD")

add_subdirectory(googletest)

enable_testing()

include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})
include_directories(${gmock_SOURCE_DIR}/include ${gmock_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../server/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../client/include)

link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../server/lib)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../client/lib)

set(SOURCE_FILES
        main.cpp
        tst_MockServer.h)

add_executable(${EXE_NAME} ${SOURCE_FILES})

target_link_libraries(${EXE_NAME} gmock gmock_main Server-lib Client-lib)

add_test(eq ${EXE_NAME})

set(SFML_STATIC_LIBRARIES TRUE)
set(SFML_ROOT "D:/Biblioteki/SFML-2.4.2")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "D:/Biblioteki/SFML-2.4.2/cmake/modules")
find_package(SFML REQUIRED system network)
if(SFML_FOUND)
  include_directories(${SFML_INCLUDE_DIR})
  target_link_libraries(${EXE_NAME} ${SFML_LIBRARIES} ${SFML_DEPENDENCIES})
endif(SFML_FOUND)
