include_directories(include)

include(FetchContent)
message("Fetching fmt")
FetchContent_Declare(fmt URL https://github.com/fmtlib/fmt/archive/refs/tags/8.0.1.tar.gz)
FetchContent_MakeAvailable(fmt)
message("Fetching Complete!")

file (GLOB_RECURSE KRILL_HEADER include/*.h)
file (GLOB_RECURSE KRILL_SRC    src/*.cpp)

add_library(krill ${KRILL_SRC} ${KRILL_HEADER})
target_link_libraries(krill PUBLIC fmt)

set(CMAKE_CXX_FLAGS   "-g")
# set(CMAKE_CXX_FLAGS   "-Wall")