project(TDGeoViewer LANGUAGES CXX)
cmake_minimum_required(VERSION 3.5)

include("../../CMake_inc.cmake")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/x11)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/x11)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/x11)
set(POWERVR_SDK_ROOT "/opt/Imagination/PowerVR_Graphics/PowerVR_SDK/SDK_2018_R2")

if (UNIX)
	add_definitions(-DUNIX)
	set(EXTRA_LIBS ${CMAKE_DL_LIBS})

	if (NOT WS)
		message("WS Variable not set. If you wish to build for X11, Wayland or another supported windowing system, please pass -DWS=X11 or -DWS=Wayland etc to CMake. At the moment only X11 build is supported.")
	endif()
	if (${WS} STREQUAL X11)
		find_package(X11 REQUIRED)
		list(APPEND EXTRA_LIBS ${X11_LIBRARIES})
		include_directories(${X11_INCLUDE_DIR})
		add_definitions(-DX11)
	endif()

endif()

set(TDGEO_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../src)

add_executable(TDGeoViewer
	../../src/TDGeometry.cpp
	src/GLDraw.cpp
	src/GLSys.cpp
	src/TDGeoViewer.cpp
)

target_include_directories (TDGeoViewer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories (TDGeoViewer PUBLIC ${TDGEO_SRC_DIR})

target_include_directories (TDGeoViewer PUBLIC "${POWERVR_SDK_ROOT}/include")
target_include_directories (TDGeoViewer PUBLIC "${POWERVR_SDK_ROOT}/external/glm")

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/shader/vtx.vert" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/vtx.vert" COPYONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/shader/hemidir.frag" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/hemidir.frag" COPYONLY)

if (EXTRA_LIBS)
	target_link_libraries(TDGeoViewer ${EXTRA_LIBS} )
endif()
