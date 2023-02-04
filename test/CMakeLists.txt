link_libraries(krill)
include_directories(${CMAKE_HOME_DIRECTORY}/krill/include)

add_executable(test_krill
	test_main.cpp
	test_automata.cpp
	test_grammar.cpp
	test_regex.cpp
	test_lexical.cpp
	test_syntax.cpp
)