#ifndef REGEX_H
#define REGEX_H
#include "defs.h"
#include <map>
#include <string>
#include <vector>
using krill::automata::DFA, krill::automata::NFA;
using krill::grammar::Token;
using std::string, std::pair, std::vector, std::map;

namespace krill::regex {

const map<int, string> regexNames = {
    {-1, "END_"},     {0, "RegEx"}, {1, "Parallel"}, {2, "'|'"},
    {3, "Seq"},       {4, "Item"},  {5, "Closure"},  {6, "Atom"},
    {7, "'+'"},       {8, "'*'"},   {9, "'?'"},      {10, "'('"},
    {11, "')'"},      {12, "Char"}, {13, "Range"},   {14, "'['"},
    {15, "RangeSeq"}, {16, "']'"},  {17, "'^'"},     {18, "RangeItem"},
    {19, "'-'"}};

DFA getDFAfromRegex(string src);
NFA getNFAfromRegex(string src);

} // namespace krill::regex

namespace krill::regex::core {

// char lexValueToChar(string lexValue);

// parse at once
NFA syntaxParser(vector<Token> tokens);
// parse at once
vector<Token> lexicalParser(string src);

} // namespace krill::regex::core
#endif