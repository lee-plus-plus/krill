link_libraries(krill)
include_directories(${CMAKE_HOME_DIRECTORY}/krill/include)

add_executable(test_automata test_automata.cpp)
add_executable(test_grammar test_grammar.cpp)
add_executable(test_codegen_c test_codegen_c.cpp)
add_executable(test_codegen_cpp test_codegen_cpp.cpp)

add_custom_target(test_codegen DEPENDS test_codegen_c test_codegen_cpp)