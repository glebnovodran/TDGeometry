project(tab2geo LANGUAGES CXX)
cmake_minimum_required(VERSION 3.5)

include("../../CMake_inc.cmake")

message(STATUS, "CMAKE_BINARY_DIR :")
message(STATUS, ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/)

set(TDGEO_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../src)

add_executable(tab2geo
	../../src/TDGeometry.cpp
	src/tab2geo.cpp
)

target_include_directories (tab2geo PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories (tab2geo PUBLIC ${TDGEO_SRC_DIR})