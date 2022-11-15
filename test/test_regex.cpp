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

void printDFA(DFA dfa, ostream &oss, bool isAscii) {
    set<int> symbolset;
    for (const auto &node : dfa.graph) {
        for (const auto &edge : node.second) { symbolset.insert(edge.first); }
    }

    oss << "DFA: \n";
    oss << "\t";
    for (int symbol : symbolset) { oss << fmt::format(isAscii ? "'{:c}'\t":"_{:d}\t", symbol); }
    oss << "\n";
    for (const auto &elem : dfa.finality) {
        const int &state = elem.first;
        oss << fmt::format("s{}", state);
        if (dfa.finality[state] != 0) {
            oss << fmt::format("(*{:d})", dfa.finality[state]);
        }
        oss << "\t";
        for (int symbol : symbolset) {
            if (dfa.graph[state].count(symbol)) {
                oss << fmt::format("s{}\t", dfa.graph[state][symbol]);
            } else {
                oss << "\t";
            }
        }
        oss << "\n";
    }
}

void printNFA(NFA nfa, ostream &oss, bool isAscii) {
    set<int> symbolset;
    for (const auto &node : nfa.graph) {
        for (const auto &edge : node.second) { symbolset.insert(edge.first); }
    }

    oss << "NFA: \n";
    oss << "\t";
    for (int symbol : symbolset) { oss << fmt::format(isAscii ? "'{:c}'\t":"_{:d}\t", symbol); }
    oss << "\n";
    for (const auto &elem : nfa.finality) {
        const int &state = elem.first;
        oss << fmt::format("s{}", state);
        if (nfa.finality[state] != 0) {
            oss << fmt::format("(*{:d})", nfa.finality[state]);
        }
        oss << "\t";
        for (int symbol : symbolset) {
            if (nfa.graph[state].count(symbol)) {
                auto it     = nfa.graph[state].lower_bound(symbol);
                auto it_end = nfa.graph[state].upper_bound(symbol);
                oss << fmt::format("s{}", it->second);
                for (it++; it != it_end; it++) {
                    oss << fmt::format(",{}", it->second);
                }
                oss << "\t";
            } else {
                oss << "\t";
            }
        }
        oss << "\n";
    }
}

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
