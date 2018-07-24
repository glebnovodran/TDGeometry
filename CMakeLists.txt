project(TDGeometry)
cmake_minimum_required(VERSION 3.5)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "./tmp")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(POWERVR_SDK_ROOT "/opt/Imagination/PowerVR_Graphics/PowerVR_SDK/SDK_2018_R1")

#add_subdirectory(src)
SET(GCC_RELEASE_COMPILE_FLAGS "-std=c++11 -g -O3 -mavx2 -mfpmath=sse -ffast-math -ftree-vectorize -mfma")
SET(GCC_RELEASE_LINK_FLAGS    "")

SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GCC_RELEASE_COMPILE_FLAGS}" )
SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${GCC_RELEASE_LINK_FLAGS}" )

add_executable(tab2geo
	src/TDGeometry.cpp
	src/tab2geo.cpp
)

add_executable(td_geo_view
	src/TDGeometry.cpp
	src/GLDraw.cpp
	src/TDGeoViewer.cpp
)
message("${POWERVR_SDK_ROOT}/include")
target_include_directories (td_geo_view PUBLIC "${POWERVR_SDK_ROOT}/include")
target_include_directories (td_geo_view PUBLIC "${POWERVR_SDK_ROOT}/external/glm")