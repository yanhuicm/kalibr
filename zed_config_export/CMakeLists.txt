CMAKE_MINIMUM_REQUIRED(VERSION 2.4)
PROJECT(export_kalibr_to_zed)

if(COMMAND cmake_policy)
	cmake_policy(SET CMP0003 OLD)
	cmake_policy(SET CMP0015 OLD)
endif(COMMAND cmake_policy)

SET(CMAKE_BUILD_TYPE "RelWithDebInfo")
SET(EXECUTABLE_OUTPUT_PATH ".")

find_package(ZED 3 REQUIRED)
find_package(CUDA ${ZED_CUDA_VERSION} EXACT REQUIRED)

include_directories(${ZED_INCLUDE_DIRS})
include_directories(${CUDA_INCLUDE_DIRS})

link_directories(${ZED_LIBRARY_DIR})

ADD_EXECUTABLE(${PROJECT_NAME} main.cpp)
add_definitions(-std=c++14)

TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${ZED_LIBRARIES})
