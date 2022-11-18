#include "krill/defs.h"
#include "krill/automata.h"
#include "fmt/format.h"
#include <iostream>
using namespace std;
using namespace krill::type;
using namespace krill::automata;

void printEdgeTable(EdgeTable edgeTable, ostream &oss, bool isAscii = false) {
    oss << "EdgeTable: \n";
    for (const Edge &edge : edgeTable) {
        oss << fmt::format(isAscii ? "s{:<2d} --> {:2c} --> s{:<2d}\n"
                                   : "s{:<2d} --> {:2d} --> s{:<2d}\n",
                           edge.from, edge.symbol, edge.to);
    }
}

void printDFA(DFA dfa, ostream &oss, bool isAscii = false) {
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

void printNFA(NFA nfa, ostream &oss, bool isAscii = false) {
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

// test the transformation from EdgeTable to NFA to DFA
void test1() {
    printf("test EdgeTable -> NFA -> DFA \n");
    printf("---------------------------- \n");
    EdgeTable edgeTable = {
        // regex: a(b|c+)c
        {'a', 0, 1}, {'b', 1, 2},  {'\0', 1, 3}, // '\0' empty edge
        {'c', 3, 3}, {'\0', 3, 2}, {'b', 2, 4},
    };
    printf("> Edge Table \n");
    printEdgeTable(edgeTable, cout);

    NFAgraph      nfaGraph = toNFAgraph(edgeTable);
    map<int, int> finality = {
        {0, 0}, {1, 0}, {2, 0}, {3, 0}, {4, 1}}; // must contains all states
    NFA nfa({nfaGraph, finality});
    printf("> NFA \n");
    printNFA(nfa, cout);

    DFA dfa = getDFAfromNFA(nfa);
    printf("> DFA \n");
    printDFA(dfa, cout);
}

// test the minimization of DFA
void test2() {
    printf("test minimization of DFA \n");
    printf("------------------------ \n");
    DFA dfa = {{
                   {0, {{'a', 11}}},
                   {11, {{'b', 2}}},
                   {22, {{'a', 33}}},
                   {33, {{'b', 0}}},
                   {44, {{'a', 55}}},
               },
               {{0, 0}, {11, 1}, {22, 0}, {33, 1}, {44, 0}, {55, 0}}};
    printf("> raw DFA \n");
    printDFA(dfa, cout);

    DFA dfa2 = getMinimizedDfa(dfa);
    printf("> minimized DFA \n");
    printDFA(dfa2, cout);
}

// test the intergration of multiple DFAs
void test3() {
    printf("test intergration of multiple DFAs \n");
    printf("---------------------------------- \n");

    DFA dfa1 = {// abcc
                {
                    {0, {{'a', 10}}},
                    {10, {{'b', 20}}},
                    {20, {{'c', 30}}},
                    {30, {{'c', 0}}},
                },
                {
                    {0, 0},
                    {10, 1},
                    {20, 0},
                    {30, 1},
                }};
    printf("> DFA(1) \n");
    printDFA(dfa1, cout);

    DFA dfa2 = {// a(ba)*
                {
                    {0, {{'a', 1}}},
                    {1, {{'b', 0}}},
                },
                {{0, 0}, {1, 1}}};
    printf("> DFA(2) \n");
    printDFA(dfa2, cout);

    DFA dfa3 = {// ac(b+c|c)
                {
                    {0, {{'a', 2}}},
                    {2, {{'c', 4}}},
                    {4, {{'b', 5}}},
                    {5, {{'b', 5}}},
                    {5, {{'c', 6}}},
                    {4, {{'c', 6}}},
                },
                {{0, 0}, {2, 0}, {4, 0}, {5, 1}, {6, 1}}};
    printf("> DFA(3) \n");
    printDFA(dfa3, cout);

    DFA dfai = getDFAintegrated({dfa1, dfa2, dfa3});
    printf("> DFA intergrated \n");
    printDFA(dfai, cout);
}

int main() {
    vector<void (*)()> testFuncs = {test1, test2, test3};
    for (int i = 0; i < testFuncs.size(); i++) {
        cout << "#test " << (i + 1) << endl;
        testFuncs[i]();
        cout << endl << endl;
    }
    return 0;
}
