project(ThreeThreadsStream)
cmake_minimum_required(VERSION 2.8)

find_package(Threads)

add_definitions(-std=c++11)
file(GLOB SRC_LIST . *)
aux_source_directory(. SRC_LIST)
add_executable(${PROJECT_NAME} ${SRC_LIST})
target_link_libraries (${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})
