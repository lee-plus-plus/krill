#include "Krill/utils.h"
#include "krill/automata.h"
#include "krill/defs.h"
#include <iostream>
using namespace std;
using namespace krill;
using namespace krill::automata;
using namespace krill::automata::core;
using namespace krill::utils;

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
