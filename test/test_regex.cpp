#include "fmt/core.h"
#include "krill/automata.h"
#include "krill/grammar.h"
#include "krill/defs.h"
#include "krill/regex.h"
#include "krill/utils.h"
#include <iostream>
#include <vector>
using namespace std;
using namespace krill::automata;
using namespace krill::grammar;
using namespace krill::regex;
using namespace krill::regex::core;
using namespace krill::utils;

static const map<int, string> symbolNames = {
    {-1, "END_"}, 
    {0, "RegEx"},   {1, "Parallel"}, {2, "'|'"},   {3, "Seq"}, {4, "Item"},
    {5, "Closure"}, {6, "Atom"},     {7, "'+'"},   {8, "'*'"}, {9, "'?'"},
    {10, "'('"},    {11, "')'"},     {12, "Char"},
};

// test the transformation from EdgeTable to NFA to DFA
void test1() {
    vector<string> srcs = {
        "abc",
        "a?b+c*d",
        "b(ac?a|b)+d",
        "(1|2)(0|1|2)*|0",
        "((1|2)(0|1|2)*|0).?(0|1|2)+",
        "(a|b|c)(a|b|c|0|1|2)*",
        " +",
    };
    for (auto src : srcs) {
        cout << fmt::format("regex: \"{}\"\n", src);

        vector<Token> tokens = lexicalParser(src);
        for (Token token : tokens) {
            cout << fmt::format("[{} '{}']", symbolNames.at(token.id),
                                token.lexValue);
        }
        cout << "\n";

        NFA nfa = syntaxParser(tokens);
        DFA dfa = getMinimizedDfa(getDFAfromNFA(nfa));
        printDFA(dfa, cout);
    }
}


int main() {
    vector<void (*)()> testFuncs = {test1};
    for (int i = 0; i < testFuncs.size(); i++) {
        cout << "#test " << (i + 1) << endl;
        testFuncs[i]();
        cout << endl << endl;
    }
    return 0;
}
