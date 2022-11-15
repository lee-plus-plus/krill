#include "Krill/utils.h"
#include "krill/codegen.h"
#include "krill/defs.h"
#include "krill/grammar.h"
#include "fmt/format.h"
#include <iostream>
using namespace std;
using krill::automata::DFA;
using krill::grammar::Grammar;
using namespace krill::codegen;


Grammar grammar = Grammar({
    "LexFile <- Defs delim Rules delim Sub eof", 
    "Defs <- DefsLine Defs", 
    "Defs <- lbrace Codes rbrace", 
    "Defs <- ", 
    "Codes <- CodesLine Codes", 
    "Codes <-", 
    "CodesLine <- line", 
    "DefsLine <- line", 
    "Rules <- RulesLine Rules ", 
    "Rules <-", 
    "RulesLine <- line ", 
    "Sub <- SubLine Sub", 
    "Sub <- ", 
    "SubLine <- line", 
});

// test Syntax Parser code generator
void test1() {
    // // fprintf(stderr, "test code generator \n");
    // // fprintf(stderr, "------------------- \n");
    
    // // fprintf(stderr, "> initial grammar (calculator): \n");
    // // printGrammar(grammar, symbolNames, cerr);

    // // generate symbol name table
    // cout << "// {\n//   ";
    // for (auto [idx, symbolName] : symbolNames) {
    //     cout << fmt::format("{{{}, \"{}\"}}, ", idx, symbolName);
    // }
    // cout << "\n// }";

    // auto actionTable = getLALR1table(grammar);
    // genSyntaxParserInCppStyle(grammar, symbolNames, actionTable, cout);
    // cout << "\nint main() { return 0; }\n";
}

void test2() {
    auto actionTable = getLALR1table(grammar);

    cout << "\n// Action Table\n";
    genActionTableInCppStyle(actionTable, cout);

    cout << "\n// Grammar\n";
    genGrammarInCppStyle(grammar, cout);
}



int main() {
    // test1();
    test2();
    return 0;
}
