#ifndef CODEGEN_H
#define CODEGEN_H
#include "defs.h"
#include <map>
#include <ostream>
#include <string>
using krill::grammar::Grammar, krill::grammar::ActionTable;
using krill::automata::DFA;
using std::map, std::string, std::ostream;

namespace krill::codegen {

void genActionTableInCppStyle(const ActionTable &actionTable, ostream &oss);
void genGrammarInCppStyle(const Grammar& grammar, ostream &oss);
void genDFAInCppStyle(const DFA &dfa, ostream &oss);

// generate code of Syntax Parser (C format, standalone)
void genSyntaxParserInCStyle(const Grammar &    grammar,
                             map<int, string>   symbolNames,
                             const ActionTable &actionTable, ostream &oss);

// generate code of Syntax Parser (C++ format, standalone)
void genSyntaxParserInCppStyle(const Grammar &    grammar,
                               map<int, string>   symbolNames,
                               const ActionTable &actionTable, ostream &oss);

// void genSyntaxParserInCppStyle(const Grammar & grammar, const ActionTable &actionTable, )
} // namespace krill::codegen
#endif