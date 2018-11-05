cmake_minimum_required(VERSION 3.5)
project(tab2geo LANGUAGES CXX)

message(STATUS, "CMAKE_BINARY_DIR :")
message(STATUS, ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/x11)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/x11)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/x11)

set (CMAKE_CXX_STANDARD 11)
set(GCC_RELEASE_COMPILE_FLAGS "-g -O3 -mavx -mfpmath=sse -ffast-math -ftree-vectorize -mfma")
set(GCC_RELEASE_LINK_FLAGS    "")

set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GCC_RELEASE_COMPILE_FLAGS}" )
set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${GCC_RELEASE_LINK_FLAGS}" )

set(TDGEO_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../src)

add_executable(tab2geo
	../../src/TDGeometry.cpp
	src/tab2geo.cpp
)

target_include_directories (tab2geo PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories (tab2geo PUBLIC ${TDGEO_SRC_DIR})