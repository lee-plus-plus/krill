#include "krill/utils.h"
#include "krill/automata.h"
#include "krill/regex.h"
#include "krill/defs.h"
#include <iostream>
using namespace std;
using namespace krill::automata;
using namespace krill::regex;
using namespace krill::utils;

// test the transformation from EdgeTable to NFA to DFA
void test1() {
    vector<string> srcs = {"abc", "a?b+c*d", "b(ac?a|b)+d", };
    for (auto src : srcs) {
        cout << "regex: " << src << endl;

        DFA dfa = getDFAfromRegex(src);
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
