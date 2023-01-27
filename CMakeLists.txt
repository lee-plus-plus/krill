cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 17)

# use g++9.0, clang++9.0 above
# cmake .. -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++

project(KRILL)

# for debug
set(CMAKE_CXX_FLAGS   "${CMAKE_CXX_FLAGS} -g")
set(CMAKE_CXX_FLAGS   "${CMAKE_CXX_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS   "${CMAKE_CXX_FLAGS} -Wno-sign-compare")
set(CMAKE_BUILD_TYPE  "Debug")

# for profile
# set(CMAKE_CXX_FLAGS   "${CMAKE_CXX_FLAGS} -pg")
# set(CMAKE_C_FLAGS     "${CMAKE_C_FLAGS} -pg")
# set(CMAKE_EXE_LINKER_FLAGS    "${CMAKE_EXE_LINKER_FLAGS} -pg")
# set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -pg")

add_subdirectory(krill)
add_subdirectory(test)
add_subdirectory(standalone)



