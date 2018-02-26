cmake_minimum_required(VERSION 3.3)

PROJECT (Wolf)

ADD_COMPILE_OPTIONS(-std=c++11 -g -Wall)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/src)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/)

SET (SRCS
        demo/cpp_demo.cpp
		src/core/area/shm_area.cpp
		src/core/container/comparator.h
		src/core/container/shm_manager.cpp
		src/core/container/random.h
		src/core/container/lockfree_linklist.h
		src/core/container/lockfree_skiplist.h src/wolf.h)

ADD_EXECUTABLE(demo ${SRCS})