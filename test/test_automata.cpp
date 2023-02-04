#include "fmt/format.h"
#include "krill/automata.h"
#include "krill/defs.h"
#include <doctest/doctest.h>
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
    for (int symbol : symbolset) {
        oss << fmt::format(isAscii ? "'{:c}'\t" : "_{:d}\t", symbol);
    }
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
    for (int symbol : symbolset) {
        oss << fmt::format(isAscii ? "'{:c}'\t" : "_{:d}\t", symbol);
    }
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
TEST_CASE("automata_NFA_to_DFA") {
    try {
        // test EdgeTable -> NFA -> DFA
        EdgeTable edgeTable = {
            // regex: a(b|c+)c
            {'a', 0, 1},  {'b', 1, 2}, {'c', 2, 3}, // '\0' empty edge
            {'\0', 1, 4}, {'c', 4, 4}, {'c', 4, 2},
        };

        NFAgraph      nfaGraph = toNFAgraph(edgeTable);
        map<int, int> finality = {
            {0, 0}, {1, 0}, {2, 0}, {3, 1}, {4, 0}}; // must contains all states
        NFA nfa({nfaGraph, finality});

        DFA dfa = getDFAfromNFA(nfa);

        CHECK(match(dfa, "abc"));
        CHECK(match(dfa, "acc"));
        CHECK(match(dfa, "acccccc"));
        CHECK(unmatch(dfa, "abbc"));
        CHECK(unmatch(dfa, "ac"));
        CHECK(unmatch(dfa, "abcc"));
        CHECK(unmatch(dfa, "acbc"));
        CHECK(unmatch(dfa, "a"));
        CHECK(unmatch(dfa, "bc"));
        CHECK(unmatch(dfa, ""));
        CHECK(unmatch(dfa, "adc"));

    } catch (exception &err) { CHECK(false); }
}

// test the minimization of DFA
TEST_CASE("automata_minimize_DFA") {
    try {
        // test minimization of DFA
        DFA dfa = {{
                       // (a)
                       {0, {{'a', 11}}},
                       {11, {{'b', 2}}},
                       {22, {{'a', 33}}},
                       {33, {{'b', 0}}},
                       {44, {{'a', 55}}},
                   },
                   {{0, 0}, {11, 1}, {22, 0}, {33, 1}, {44, 0}, {55, 0}}};
        DFA dfa2 = getMinimizedDfa(dfa);

        CHECK(match(dfa, "a"));
        CHECK(unmatch(dfa, "ab"));
        CHECK(unmatch(dfa, "aba"));
        CHECK(unmatch(dfa, ""));

        CHECK(match(dfa2, "a"));
        CHECK(unmatch(dfa2, "ab"));
        CHECK(unmatch(dfa2, "aba"));
        CHECK(unmatch(dfa2, ""));

    } catch (std::runtime_error &err) { CHECK(false); }
}

// test the intergration of multiple DFAs
TEST_CASE("automata_intergrate_DFAs") {
    try {
        // test intergration of multiple DFAs
        DFA dfa1 = {// abcc
                    {
                        {0, {{'a', 10}}},
                        {10, {{'b', 20}}},
                        {20, {{'c', 30}}},
                        {30, {{'c', 40}}},
                    },
                    {
                        {0, 0},
                        {10, 0},
                        {20, 0},
                        {30, 0},
                        {40, 1},
                    }};

        CHECK(match(dfa1, "abcc"));
        CHECK(unmatch(dfa1, "abc"));
        CHECK(unmatch(dfa1, "ac"));
        CHECK(unmatch(dfa1, "abbc"));

        DFA dfa2 = {// a(ba)*
                    {
                        {0, {{'a', 1}}},
                        {1, {{'b', 0}}},
                    },
                    {{0, 0}, {1, 1}}};

        CHECK(match(dfa2, "a"));
        CHECK(match(dfa2, "aba"));
        CHECK(match(dfa2, "abababa"));
        CHECK(unmatch(dfa2, "ab"));
        CHECK(unmatch(dfa2, "abab"));
        CHECK(unmatch(dfa2, "abb"));

        DFA dfa3 = {// ac(b+c|c)
                    {
                        {0, {{'a', 2}}},
                        {2, {{'c', 4}}},
                        {4, {{'b', 5}, {'c', 6}}},
                        {5, {{'b', 5}, {'c', 6}}},
                    },
                    {{0, 0}, {2, 0}, {4, 0}, {5, 0}, {6, 1}}};
        
        CHECK(match(dfa3, "acc"));
        CHECK(match(dfa3, "acbc"));
        CHECK(match(dfa3, "acbbbbc"));
        CHECK(unmatch(dfa3, "acb"));
        CHECK(unmatch(dfa3, "acbcb"));
        CHECK(unmatch(dfa3, "abc"));

        DFA dfai = getDFAintegrated({dfa1, dfa2, dfa3});
        
        CHECK(match(dfai, "abcc") == 1);
        CHECK(match(dfai, "a") == 2);
        CHECK(match(dfai, "aba") == 2);
        CHECK(match(dfai, "abababa") == 2);
        CHECK(match(dfai, "acc") == 3);
        CHECK(match(dfai, "acbc") == 3);
        CHECK(match(dfai, "acbbbbc") == 3);

    } catch (std::runtime_error &err) { CHECK(false); }
}
