#ifndef REGEX_H
#define REGEX_H
#include "defs.h"
#include <string>
#include <vector>
using krill::automata::DFA, krill::automata::NFA;
using std::string, std::pair, std::vector;

namespace krill::regex {

DFA getDFAfromRegex(string src);
NFA getNFAfromRegex(string src);

} // namespace krill::regex

namespace krill::regex::core {

NFA syntaxParser(vector<int> tokens, vector<string> lexValues);
pair<vector<int>, vector<string>> lexicalParser(string src);

} // namespace krill::regex::core
#endif