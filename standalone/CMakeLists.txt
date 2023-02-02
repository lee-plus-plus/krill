link_libraries(krill)
include_directories(${CMAKE_HOME_DIRECTORY}/krill/include)

add_executable(kriller kriller.cpp)
add_executable(calc  calc.cpp)
add_executable(calc2  calc2.cpp)

# set_target_properties(kriller PROPERTIES OUTPUT_NAME krill) # rename