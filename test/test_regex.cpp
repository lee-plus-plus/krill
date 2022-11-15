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

void test1() {
    vector<string> regexs = {
        "abc",
        "a?b+c*d",
        "b(ac?a|b)+d",
        "(1|2)(0|1|2)*|0",
        "[0-2]",
        "[^a-y]",
        "[^a-zA-Z0-9]",
        "\\n",
    };
    for (auto regex : regexs) {
        cout << fmt::format("regex: \"{}\"\n", regex);
        DFA dfa = getDFAfromRegex(regex);
        printDFA(dfa, cout, false);
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
