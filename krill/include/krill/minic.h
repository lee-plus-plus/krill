#ifndef MINIC_H
#define MINIC_H
#include "defs.h"
#include "syntax.h"
#include "lexical.h"
#include <memory>
#include <ostream>
#include <stack>
#include <deque>
#include <string>
using krill::type::Grammar;
using krill::runtime::SyntaxParser, krill::runtime::LexicalParser;

namespace krill::minic {

extern Grammar minicGrammar;
extern SyntaxParser minicSyntaxParser;
extern LexicalParser minicLexicalParser;
extern int getMinicSyntaxId(Token token);

} // namespace krill::minic
#endif