#ifndef UTILS_H
#define UTILS_H
#include "defs.h"
#include <map>
#include <ostream>
#include <set>
#include <string>
#include <vector>
using std::ostream;
using std::pair;
using std::string;
using std::vector, std::map, std::set;
using namespace krill::automata;
using namespace krill::grammar;

namespace krill::utils {

void rtrim(string &str);
vector<string> split(string str, const char *delim);

// template <typename T1, typename T2>
// map<T2, T1> reverse(map<T1, T2> m);

void printEdgeTable(EdgeTable edgeTable, ostream &oss, bool isAscii = false);
void printDFA(DFA dfa, ostream &oss, bool isAscii = false);
void printNFA(NFA nfa, ostream &oss, bool isAscii = false);

void printProd(const Prod &prod, map<int, string> symbolNames, ostream &oss);
void printGrammar(const Grammar &grammar, map<int, string> symbolNames,
                  ostream &oss);
void printFirstSet(const map<int, set<int>> &firstSet, ostream &oss);

void printActionTable(const ActionTable &actionTable,
                      map<int, string> symbolNames, ostream &oss);

void printToken(Token token, ostream &oss);

// grammar string => Grammar, Symbol Names
// pair<Grammar, map<int, string>> getGrammarFromStr(vector<string> prodStrs);

} // namespace krill::utils
#endif