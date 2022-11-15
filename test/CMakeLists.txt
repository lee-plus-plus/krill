link_libraries(krill)
include_directories(${CMAKE_HOME_DIRECTORY}/krill/include)

add_executable(test_automata test_automata.cpp)
add_executable(test_grammar test_grammar.cpp)
add_executable(test_codegen_c test_codegen_c.cpp)
add_executable(test_codegen_cpp test_codegen_cpp.cpp)
add_executable(test_codegen_regex test_codegen_regex.cpp)
add_executable(test_regex test_regex.cpp)
add_executable(test_lexical test_lexical.cpp)
add_executable(test_syntax test_syntax.cpp)
add_executable(test_codegen_c9 test_codegen_c9.cpp)

add_custom_target(test_codegen 
	DEPENDS test_codegen_c test_codegen_cpp test_codegen_regex
	COMMAND ${CMAKE_COMMAND} -E make_directory ../demo
	COMMAND test_codegen_c   > ../demo/calc.c
	COMMAND test_codegen_cpp > ../demo/calc.cpp
	COMMAND test_codegen_regex > ../demo/regex.cpp
	COMMENT "generated as demo/calc.c, demo/calc.cpp, demo/regex.cpp"
	)


