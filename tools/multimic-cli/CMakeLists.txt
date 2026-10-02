cmake_minimum_required(VERSION 3.16)
project(multimic-cli CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../server/include)

add_executable(multimic-cli main.cpp ../../server/src/protocol.cpp)
