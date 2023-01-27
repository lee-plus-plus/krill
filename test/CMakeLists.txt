link_libraries(krill)
include_directories(${CMAKE_HOME_DIRECTORY}/krill/include)

add_executable(test_automata test_automata.cpp)
add_executable(test_grammar test_grammar.cpp)
add_executable(test_regex test_regex.cpp)
add_executable(test_lexical test_lexical.cpp)
add_executable(test_syntax test_syntax.cpp)