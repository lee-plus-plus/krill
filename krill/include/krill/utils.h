#ifndef UTILS_H
#define UTILS_H
#include "defs.h"
#include <ostream>
#include <string>
#include <vector>
#include <map>
#include <set>
using std::ostream;
using std::string;
using std::pair;
using std::vector, std::map, std::set;
using namespace krill::automata;
using namespace krill::grammar;

namespace krill::utils {

void printEdgeTable(EdgeTable edgeTable, ostream &oss);
void printDFA(DFA dfa, ostream &oss);
void printNFA(NFA nfa, ostream &oss);

void printProd(const Prod &prod, map<int, string> symbolNames, ostream &oss);
// void printProdItem(const ProdItem &prodItem, ostream &oss);
// void printProdLR1Item(const ProdLR1Item &prodItem, ostream &oss);
void printGrammar(const Grammar &grammar, map<int, string> symbolNames,
                  ostream &oss);
void printFirstset(const map<int, set<int>> &firstSet, ostream &oss);

void printActionTable(const ActionTable &actionTable, map<int, string> symbolNames, ostream &oss);

pair<Grammar, map<int, string>> getGrammarFromStr(vector<string> prodStrs);

} // namespace krill::utils
#endif