#include "Krill/utils.h"
#include "krill/codegen.h"
#include "krill/defs.h"
#include "krill/grammar.h"
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
        "Q -> P",
        "P -> T",
        "T -> ( P )",
        "T -> T * T",
        "T -> T / T",
        "P -> T + T",
        "P -> T - T",
        "T -> - T",
        "T -> d",
    });
    // fprintf(stderr, "> initial grammar (calculator): \n");
    // printGrammar(grammar, symbolNames, cerr);

    auto actionTable = getLALR1table(grammar);
    genSyntaxParserInCStyle(grammar, symbolNames, actionTable, cout);
    cout << "\nint main() { return 0; }\n";
}


int main() {
    test1();
    return 0;
}
