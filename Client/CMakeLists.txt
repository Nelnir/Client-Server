cmake_minimum_required(VERSION 3.10.0)
project(Client)

#set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
set(EXE_NAME Client)
set(LIB_NAME Client-lib)


# Output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
set(EXE_SOURCES src/main.cpp src/consoleclient.cpp include/consoleclient.h)

add_executable(${EXE_NAME} ${EXE_SOURCES})

add_library(${LIB_NAME} src/client.cpp include/client.h)

target_link_libraries(${EXE_NAME} ${LIB_NAME})

set(SFML_STATIC_LIBRARIES TRUE)
set(SFML_ROOT "D:/Biblioteki/SFML-2.4.2")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "D:/Biblioteki/SFML-2.4.2/cmake/modules")
find_package(SFML REQUIRED system network)
if(SFML_FOUND)
  include_directories(${SFML_INCLUDE_DIR})
  target_link_libraries(${EXE_NAME} ${SFML_LIBRARIES} ${SFML_DEPENDENCIES})
endif(SFML_FOUND)
