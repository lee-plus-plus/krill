#include "fmt/format.h"
#include "krill/defs.h"
#include "krill/automata.h"
#include "krill/grammar.h"
#include "krill/regex.h"
#include "krill/codegen.h"
#include <iostream>
#include <sstream>
#include <map>
#include <vector>
using namespace std;
using namespace krill::automata;
using namespace krill::grammar;
using namespace krill::regex;
using namespace krill::utils;
using namespace krill::codegen;
using namespace krill::runtime;

Grammar grammar = Grammar({
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
    "Atom -> Range", 
    "Range -> '[' RangeSeq ']'", 
    "Range -> '[' '^' RangeSeq ']'", 
    "RangeSeq -> RangeSeq RangeItem", 
    "RangeSeq -> RangeItem", 
    "RangeItem -> Char '-' Char", 
    "RangeItem -> Char", 
    "Atom -> '.'", 
});

void test2() {
    auto actionTable = getLALR1table(grammar);

    cout << "\n// Action Table\n";
    genActionTableInCppStyle(actionTable, cout);

    cout << "\n// Grammar\n";
    genGrammarInCppStyle(grammar, cout);
}

void test() {
    vector<string> grammarStrs;
    while (true) {
        string line;
        getline(cin, line);
        if (cin.eof()) {
            break;
        }
        grammarStrs.push_back(line);
    }

    Grammar grammar(grammarStrs);
    auto actionTable = getLALR1table(grammar);

    cout << "\n// Action Table\n";
    genActionTable(actionTable, cout);

    cout << "\n// Grammar\n";
    genGrammar(grammar, cout);
}


int main() {
    // test1();
    // test2();
    test();
    return 0;
}
