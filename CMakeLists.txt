cmake_minimum_required(VERSION 3.3)

PROJECT (Wolf)

ADD_COMPILE_OPTIONS(-std=c++11 -g -Wall)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/)

SET (SRCS
        demo/cpp_demo.cpp
        area/shm_area.cpp
        container/comparator.h
		container/shm_manager.cpp
        container/random.h
        container/lockfree_linklist.h
        container/lockfree_skiplist.h)

ADD_EXECUTABLE(demo ${SRCS})