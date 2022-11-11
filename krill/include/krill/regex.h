#ifndef REGEX_H
#define REGEX_H
#include "defs.h"
#include <string>
#include <vector>
using krill::automata::DFA, krill::automata::NFA;
using krill::grammar::Token;
using std::string, std::pair, std::vector;

namespace krill::regex {

DFA getDFAfromRegex(string src);
NFA getNFAfromRegex(string src);

} // namespace krill::regex

namespace krill::regex::core {

// parse at once
NFA syntaxParser(vector<Token> tokens);
// parse at once
vector<Token> lexicalParser(string src);

} // namespace krill::regex::core
#endif