cmake_minimum_required(VERSION 3.10.0)
project(Server)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
set(EXE_NAME Server)
set(LIB_NAME nelnir-s)

# Output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib)
message(${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
set(EXE_SOURCES src/main.cpp)
set(LIB_SOURCES src/Server.cpp src/ConsoleServer.cpp)

add_executable(${EXE_NAME} ${EXE_SOURCES})
add_library(${LIB_NAME} ${LIB_SOURCES})

set(SFML_STATIC_LIBRARIES TRUE)
set(SFML_ROOT "D:/Biblioteki/SFML-2.4.2")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "D:/Biblioteki/SFML-2.4.2/cmake/modules")
find_package(SFML REQUIRED system network)
if(SFML_FOUND)
  include_directories(${SFML_INCLUDE_DIR})
  target_link_libraries(${LIB_NAME} ${SFML_LIBRARIES} ${SFML_DEPENDENCIES})
  target_link_libraries(${EXE_NAME} ${LIB_NAME})
endif(SFML_FOUND)