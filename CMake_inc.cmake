set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(GCC_RELEASE_COMPILE_FLAGS "-O3 -mavx2 -mfpmath=sse -ffast-math -ftree-vectorize -mfma")

set(GCC_DEBUG_COMPILE_FLAGS " -ggdb -mavx2 -mfpmath=sse -ffast-math -ftree-vectorize -mfma")

set (CMAKE_CXX_FLAGS_RELEASE ${GCC_RELEASE_COMPILE_FLAGS} )
set(CMAKE_EXE_LINKER_FLAGS_RELEASE  "${CMAKE_EXE_LINKER_FLAGS} ${GCC_RELEASE_COMPILE_FLAGS}" )

set (CMAKE_CXX_FLAGS_DEBUG ${GCC_DEBUG_COMPILE_FLAGS})
set(CMAKE_EXE_LINKER_FLAGS_DEBUG  "${CMAKE_EXE_LINKER_FLAGS} ${GCC_DEBUG_COMPILE_FLAGS}" )

set(default_build_type "Release")

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE
      string "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
	"Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()