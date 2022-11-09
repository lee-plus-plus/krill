#ifndef CODEGEN_H
#define CODEGEN_H
#include "defs.h"
#include <map>
#include <string>
#include <ostream>

namespace krill::codegen {
    using krill::grammar::Grammar, krill::grammar::ActionTable;
    using std::map, std::string, std::ostream;
    // generate code of Syntax Parser (C format, standalone)
    void genSyntaxParserInCStyle(
        const Grammar &grammar, map<int, string> symbolNames,
        const ActionTable &actionTable, ostream &oss);

    // generate code of Syntax Parser (C++ format, standalone)
    void genSyntaxParserInCppStyle(
        const Grammar &grammar, map<int, string> symbolNames,
        const ActionTable &actionTable, ostream &oss);
} // namespace krill::export
#endif