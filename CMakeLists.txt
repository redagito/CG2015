# Minimum cmake version
cmake_minimum_required(VERSION 3.0)

# Project name
project(CG2015)

# Include
include_directories(
)

# Sources
file(GLOB_RECURSE SRC_FILES
	${CMAKE_SOURCE_DIR}/source/*.cpp
	${CMAKE_SOURCE_DIR}/source/*.h
)

# Flags
# TODO Only for g++
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

# Add library
add_executable(CG2015 ${SRC_FILES})