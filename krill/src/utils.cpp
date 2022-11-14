#include "krill/utils.h"
#include "krill/automata.h"
#include "krill/grammar.h"
#include <cstring>
#include <map>
#include <string>
#include <fmt/core.h>
#include <set>
using namespace krill::automata;
using namespace krill::grammar;
using namespace std;

namespace krill::utils {

void rtrim(string &str) { str.erase(str.find_last_not_of("\r\n\0") + 1); }

vector<string> split(string str, const char *delim) {
    vector<string> res;
    char *         strc = &str[0];
    char *         temp = strtok(strc, delim);
    while (temp != NULL) {
        res.push_back(string(temp));
        temp = strtok(NULL, delim);
    }
    return res;
}

// template <typename T1, typename T2>
// map<T2, T1> reverse(map<T1, T2> m) {
//     map<T2, T1> m_reversed;
//     for (auto [key, value] : m) {
//         m_reversed[value] = key;
//     }
//     return m_reversed;
// }

void printEdgeTable(EdgeTable edgeTable, ostream &oss, bool isAscii) {
    oss << "EdgeTable: \n";
    for (const Edge &edge : edgeTable) {
        oss << fmt::format(isAscii ? "s{:<2d} --> {:2c} --> s{:<2d}\n"
                                   : "s{:<2d} --> {:2d} --> s{:<2d}\n",
                           edge.from, edge.symbol, edge.to);
    }
}

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

void printProd(const Prod &prod, map<int, string> symbolNames, ostream &oss) {
    oss << symbolNames[prod.symbol];
    oss << " -> ";
    for (int i = 0; i < prod.right.size(); i++) {
        oss << symbolNames[prod.right[i]] << " ";
    }
    oss << "\n";
}

void printGrammar(const Grammar &grammar, map<int, string> symbolNames,
                  ostream &oss) {
    oss << "Grammar: \n";
    for (int i = 0; i < grammar.prods.size(); i++) {
        oss << fmt::format("({:d}) ", i + 1);
        printProd(grammar.prods[i], symbolNames, oss);
    }
}

void printFirstSet(const map<int, set<int>> &firstSet, ostream &oss) {
    for (auto[symbol, nextSymbols] : firstSet) {
        oss << symbol;
        oss << ": {";
        if (nextSymbols.size() != 0) {
            auto it = nextSymbols.begin();
            oss << *it;
            for (it++; it != nextSymbols.end(); it++) {
                oss << ", ";
                oss << *it;
            }
        }
        oss << "} \n";
    }
}

void printActionTable(const ActionTable &actionTable,
                      map<int, string> symbolNames, ostream &oss) {
    oss << fmt::format("Analysis Table (size={}): \n", actionTable.size());
    string typeName[] = {"ACTION", "REDUCE", "GOTO  ", "ACCEPT"};
    for (auto[key, action] : actionTable) {
        oss << fmt::format("s{:<2d} --> {:s} --> {:<6s} ", key.first,
                           key.second != END_SYMBOL ? symbolNames[key.second]
                                                    : "[end]",
                           typeName[action.type]);
        if (action.type == Action::ACTION || action.type == Action::GOTO) {
            oss << fmt::format("s{:<2d}", action.tgt);
        } else if (action.type == Action::REDUCE) {
            oss << fmt::format("r{:<2d}", action.tgt);
        }
        oss << "\n";
    }
}

void printToken(Token token, ostream &oss) {
    oss << fmt::format("[{} \"{}\"", token.id, token.lexValue);
}


pair<Grammar, map<int, string>> getGrammarFromStr(vector<string> prodStrs) {
    vector<Prod>     prods;
    map<string, int> symbolNamesRev;
    map<int, string> symbolNames;
    int              numStates = 0;

    for (string prodStr : prodStrs) {
        vector<string> words = split(prodStr, " ");
        // assume words like {"Term", "->", "Term", "add", "D-Term"}
        // throw away the second element "->"
        int         symbol;
        vector<int> right;
        for (int i = 0; i < words.size(); i++) {
            if (i == 1) { continue; }
            if (symbolNamesRev.count(words[i]) == 0) {
                symbolNamesRev[words[i]] = numStates;
                symbolNames[numStates]   = words[i];
                numStates++;
            }
            if (i == 0) {
                symbol = symbolNamesRev[words[i]];
            } else {
                right.push_back(symbolNamesRev[words[i]]);
            }
        }
        prods.push_back({symbol, right});
    }

    symbolNames[END_SYMBOL] = "END_";
    return {Grammar(prods), symbolNames};
}

} // namespace krill::utils