project(TDGeometry)
cmake_minimum_required(VERSION 3.5)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set (CMAKE_CXX_STANDARD 11)
set(GCC_RELEASE_COMPILE_FLAGS "-std=c++11 -g -O3 -mavx2 -mfpmath=sse -ffast-math -ftree-vectorize -mfma")
set(GCC_RELEASE_LINK_FLAGS    "")

set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GCC_RELEASE_COMPILE_FLAGS}" )
set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${GCC_RELEASE_LINK_FLAGS}" )

add_subdirectory("./samples/tab2geo")
add_subdirectory("./samples/TDGeoViewer")
#add_executable(td_geo_view
#	src/TDGeometry.cpp
#	src/GLDraw.cpp
#	src/TDGeoViewer.cpp
#)

#message("${POWERVR_SDK_ROOT}/include")
#target_include_directories (td_geo_view PUBLIC "${POWERVR_SDK_ROOT}/include")
#target_include_directories (td_geo_view PUBLIC "${POWERVR_SDK_ROOT}/external/glm")
