# Minimum cmake version
cmake_minimum_required(VERSION 2.8)

# Project name
project(CG2015)

# Required packages
find_package (OpenGL REQUIRED)

# GLFW options
option(GLFW_BUILD_DOCS OFF)
option(GLFW_BUILD_EXAMPLES OFF)
option(GLFW_BUILD_TESTS OFF)
option(GLFW_INSTALL OFF)

# Add glfw
add_subdirectory(external/glfw)
include_directories(external/glfw/include)

# Add tinyobj
add_subdirectory(external/tinyobj)
include_directories(external/tinyobj)

# JsonCpp options
option(JSONCPP_WITH_TESTS OFF)
option(JSONCPP_WITH_POST_BUILD_UNITTEST OFF)
option(JSONCPP_WITH_PKGCONFIG_SUPPORT OFF)
option(JSONCPP_WITH_WARNING_AS_ERROR OFF)

# Add jsoncpp
add_subdirectory(external/jsoncpp)
include_directories(external/jsoncpp/include)

# Add glm
# Fix for 32 bit alignment issue in 9.6.0 branch
add_definitions(-DGLM_FORCE_PURE)
include_directories(external/glm)

# Add lodepng
file(GLOB LODEPNG_SOURCES ${CMAKE_SOURCE_DIR}/external/lodepng/lodepng.cpp)
set(THIRD_PARTY_SOURCES ${THIRD_PARTY_SOURCES} ${LODEPNG_SOURCES})
include_directories(external/lodepng)

# Add flextgl 4.3 core
file(GLOB FLEXTGL_SOURCES ${CMAKE_SOURCE_DIR}/external/flextgl/*.cpp)
set(THIRD_PARTY_SOURCES ${THIRD_PARTY_SOURCES} ${FLEXTGL_SOURCES})
include_directories(external/flextgl)

# Add irrklang
include_directories(external/irrklang/irrKlang-1.5.0/include)

# Windows only, TODO fix
link_directories(
	external/irrklang/irrKlang-1.5.0/lib/Win32-visualStudio
	external/irrklang/irrKlang-1.5.0/lib/Win32-gcc
)

# Add zlib
add_subdirectory(external/zlib)
include_directories(externa/zlib)

# Include
include_directories(
	${CMAKE_SOURCE_DIR}/source/
	${CMAKE_BINARY_DIR}/source/
)

# Sources
file(GLOB_RECURSE SRC_FILES
	${CMAKE_SOURCE_DIR}/source/*.cpp
	${CMAKE_SOURCE_DIR}/source/*.h
	${CMAKE_BINARY_DIR}/source/*.cpp
	${CMAKE_BINARY_DIR}/source/*.h
)

# Add third party sources
set(SRC_FILES ${SRC_FILES} ${THIRD_PARTY_SOURCES})

# Flags
# TODO Only for g++
if (UNIX)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDEBUG")
endif (UNIX)


# Add library
add_executable(CG2015 ${SRC_FILES})

# Link dependencies
target_link_libraries(CG2015
	${OPENGL_LIBRARY}
	glfw
	${GLFW_LIBRARIES}
    tinyobjloader
    jsoncpp_lib_static
	zlibstatic
)

if (UNIX)
target_link_libraries(CG2015
	dl
)
endif (UNIX)

if (WIN32)
target_link_libraries(CG2015
	irrKlang
)
endif (WIN32)
