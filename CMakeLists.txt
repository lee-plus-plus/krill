cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 17)

project(KRILL)

# for debug
set(CMAKE_CXX_FLAGS   "${CMAKE_CXX_FLAGS} -g")
set(CMAKE_CXX_FLAGS   "${CMAKE_CXX_FLAGS} -Wall")
set(CMAKE_BUILD_TYPE  "Debug")

add_subdirectory(krill)
add_subdirectory(test)



