cmake_minimum_required(VERSION 3.13)
project(PackQueue)

set(CMAKE_CXX_STANDARD 14)
#set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG=1 -lgtest")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -lgtest -lpthread")

add_executable(PackQueue main.cpp PackQueue.h)