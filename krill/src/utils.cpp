#include "krill/utils.h"
#include "krill/automata.h"
#include <fmt/core.h>
#include <set>
using namespace krill::automata;

namespace krill::utils {

void printEdgeTable(EdgeTable edgeTable, ostream &oss) {
    oss << "EdgeTable: \n";
    for (const Edge &edge : edgeTable) {
        oss << fmt::format("s{:<2d} --> {:2d} --> s{:<2d}\n", 
                           edge.from, edge.symbol, edge.to);
    }
}

void printDFAasTable(DFA dfa, ostream &oss) {
    set<int> symbolset;
    for (const auto &node : dfa.graph) {
        for (const auto &edge : node.second) { symbolset.insert(edge.first); }
    }

    oss << "DFA: \n";
    oss << "\t";
    for (int symbol : symbolset) { oss << "_" << symbol << "\t"; }
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

void printNFAasTable(NFA nfa, ostream &oss) {
    set<int> symbolset;
    for (const auto &node : nfa.graph) {
        for (const auto &edge : node.second) { symbolset.insert(edge.first); }
    }

    oss << "NFA: \n";
    oss << "\t";
    for (int symbol : symbolset) { oss << "_" << symbol << "\t"; }
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
                oss << fmt::format("s{}\t", it->second);
                for (it++; it != it_end; it++) {
                    oss << fmt::format(",s{}\t", it->second);
                }
            } else {
                oss << "\t";
            }
        }
        oss << "\n";
    }
}

} // namespace krill::utils