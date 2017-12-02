project(extractor)
cmake_minimum_required(VERSION 3.0)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -O0")

find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin )

include_directories (${extractor_SOURCE_DIR}/src)
set (
  extractor_SOURCES
  ${extractor_SOURCE_DIR}/src/json.hpp
  ${extractor_SOURCE_DIR}/src/main.cpp
)
add_executable (extractor ${extractor_SOURCES})
target_link_libraries(extractor ${OpenCV_LIBS})

message (STATUS "Built: " )
