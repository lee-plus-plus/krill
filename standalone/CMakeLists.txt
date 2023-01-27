link_libraries(krill)
include_directories(${CMAKE_HOME_DIRECTORY}/krill/include)

add_executable(kriller kriller.cpp)
add_executable(mico  mico.cpp)
add_executable(calc  calc.cpp)