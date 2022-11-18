#ifndef CODEGEN_H
#define CODEGEN_H
#include "defs.h"
#include <map>
#include <ostream>
#include <string>
#include <vector>
using krill::type::Grammar, krill::type::ActionTable, krill::type::DFA;
using std::map, std::string, std::ostream, std::vector;

namespace krill::codegen {

void genActionTable(const ActionTable &actionTable, ostream &oss);
void genGrammar(const Grammar& grammar, ostream &oss);
void genDFA(const DFA &dfa, ostream &oss);

void genSyntaxParser(const Grammar& grammar, ostream &oss);
void genLexicalParser(const vector<string> &regexs, ostream &oss);

// void genSyntaxParserInCppStyle(const Grammar & grammar, const ActionTable &actionTable, )
} // namespace krill::codegen
#endif