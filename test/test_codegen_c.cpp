#include "krill/defs.h"
#include "krill/grammar.h"
#include "krill/codegen.h"
#include "Krill/utils.h"
#include <iostream>
using namespace std;
using namespace krill;
using namespace krill::automata;
using namespace krill::grammar;
using namespace krill::utils;
using namespace krill::codegen;

// test the transformation from EdgeTable to NFA to DFA
void test1() {
	fprintf(stderr, "test code generator \n");
    fprintf(stderr, "------------------- \n");
	auto[grammar, symbolNames] = getGrammarFromStr({
        "Q -> S",
        "S -> V = R",
        "S -> R",
        "R -> L",
        "L -> * R",
        "L -> i",
        "V -> a",
    });
    fprintf(stderr, "> initial grammar: \n");
    printGrammar(grammar, symbolNames, cerr);

    auto actionTable = getLALR1table(grammar);
    genSyntaxParserInCStyle(grammar, symbolNames, actionTable, cout);
    cout << "\nint main() { return 0; }\n";
}


int main() {
	vector<void(*)()> testFuncs = {test1, };
	for (int i = 0; i < testFuncs.size(); i++) {
		cerr << "#test " << (i+1) << endl;
		testFuncs[i]();
		cerr << endl << endl;
	}
	return 0;
}
