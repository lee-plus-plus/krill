#include "fmt/format.h"
#include "krill/defs.h"
#include "krill/grammar.h"
#include "krill/regex.h"
#include "krill/codegen.h"
#include <iostream>
#include <sstream>
#include <map>
#include <vector>
using namespace std;
using namespace krill::grammar;
using namespace krill::regex;
using namespace krill::utils;
using namespace krill::codegen;
using namespace krill::runtime;

Grammar grammar({
    "translation_unit : external_declaration", 
    "translation_unit : translation_unit external_declaration", 
    "external_declaration : function_definition", 
    "external_declaration : declaration", 
    "function_definition : declaration_specifiers IDENTIFIER '(' ')' compound_statement", 
    "compound_statement : '{' '}'", 
    "compound_statement : '{' block_item_list '}'", 
    "block_item_list : block_item", 
    "block_item_list : block_item_list block_item", 
    "block_item : declaration", 
    "declaration : declaration_specifiers init_declarator_list ';'", 
    "declaration_specifiers : INT", 
    "init_declarator_list : init_declarator", 
    "init_declarator_list : init_declarator_list ',' init_declarator", 
    "init_declarator : IDENTIFIER", 
    "init_declarator : IDENTIFIER '=' assignment_expression", 
    "assignment_expression : IDENTIFIER", 
    "assignment_expression : CONSTANT", 
    "assignment_expression : CONSTANT : STRING_LITERAL", 
});

vector<string> regexs = {
    R"(int)", 
    R"([0-9]+)", 
    R"([a-zA-Z_]([a-zA-Z_]|[0-9])*)", 
    // R"(L?\"(\\.|[^\\\"\r\n])*\")", 
    // R"(;)", 
    // R"(({|<%))", 
    // R"((}|%>))", 
    // R"(=)", 
    // R"(\()", 
    R"(\))", 
    R"(\-)", 
    R"(\+)", 
    // R"([ \t\v\r\n\f])", 
    R"(.)", 
};


int main() {
    ActionTable actionTable = getLALR1table(grammar);
    vector<DFA> dfas;
    for (string regex : regexs) {
    	dfas.push_back(getDFAfromRegex(regex));
    }
    DFA dfai = getDFAintegrated(dfas);

    cout << "\n// Action Table\n";
    genActionTableInCppStyle(actionTable, cout);
    cout << "\n// Grammar\n";
	genGrammarInCppStyle(grammar, cout);
	cout << "\n// DFA\n";
	genDFAInCppStyle(dfai, cout);

    return 0;
}
