#include "Krill/utils.h"
#include "krill/codegen.h"
#include "krill/defs.h"
#include "krill/grammar.h"
#include "fmt/format.h"
#include <iostream>
using namespace std;
using namespace krill;
using namespace krill::automata;
using namespace krill::grammar;
using namespace krill::utils;
using namespace krill::codegen;

// test Syntax Parser code generator
void test1() {
    // fprintf(stderr, "test code generator \n");
    // fprintf(stderr, "------------------- \n");
    auto[grammar, symbolNames] = getGrammarFromStr({
        "RegEx -> Parallel", 
        "Parallel -> Parallel '|' Seq", 
        "Parallel -> Seq", 
        "Seq -> Seq Item", 
        "Seq -> Item ", 
        "Item -> Closure", 
        "Item -> Atom", 
        "Closure -> Atom '+'", 
        "Closure -> Atom '*'", 
        "Closure -> Atom '?'", 
        "Atom -> '(' Parallel ')'", 
        "Atom -> Char", 
    });
    // fprintf(stderr, "> initial grammar (calculator): \n");
    // printGrammar(grammar, symbolNames, cerr);

    // generate symbol name table
    cout << "// {\n//   ";
    for (auto [idx, symbolName] : symbolNames) {
        cout << fmt::format("{{{}, \"{}\"}}, ", symbolName, idx);
    }
    cout << "\n// }";

    auto actionTable = getLALR1table(grammar);
    genSyntaxParserInCppStyle(grammar, symbolNames, actionTable, cout);
    cout << "\nint main() { return 0; }\n";

}


int main() {
    test1();
    return 0;
}
