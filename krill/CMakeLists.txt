include_directories(include)

include(FetchContent)

# Use fmt for formatting string.
message("Fetching fmt")
FetchContent_Declare(fmt URL https://github.com/fmtlib/fmt/archive/refs/tags/8.0.1.tar.gz)
FetchContent_MakeAvailable(fmt)
message("Fetching Complete!")

# Use SPDLOG for logging.
message("Fetching SPDLOG")
FetchContent_Declare(
  spdlog URL https://github.com/gabime/spdlog/archive/refs/tags/v1.10.0.tar.gz
)
set(SPDLOG_FMT_EXTERNAL ON) # to resolve confilt between spdlog and fmt
FetchContent_MakeAvailable(spdlog)
message("Fetch Complete!")

# Use magic_enum to support static reflection for enums.
message("Fetching magic_enum")
FetchContent_Declare(
  magic_enum URL https://github.com/Neargye/magic_enum/archive/refs/tags/v0.8.1.tar.gz
)
FetchContent_MakeAvailable(magic_enum)
message("Fetching Complete!")

file (GLOB_RECURSE KRILL_HEADER include/*.h)
file (GLOB_RECURSE KRILL_SRC    src/*.cpp)

add_library(krill ${KRILL_SRC} ${KRILL_HEADER})
target_link_libraries(krill PUBLIC fmt)
target_link_libraries(krill PUBLIC spdlog::spdlog magic_enum::magic_enum)
