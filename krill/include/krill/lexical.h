#ifndef LEXICAL_H
#define LEXICAL_H
#include "automata.h"
#include "grammar.h"
#include "defs.h"
#include <istream>
#include <string>
#include <map>
#include <vector>
using krill::automata::DFA;
using krill::grammar::Grammar, krill::grammar::Token;
using std::pair, std::map, std::vector, std::string, std::istream;

namespace krill::runtime {

class BaseLexicalParser {
  protected:
    DFA dfa_;
    int state_;

  public:
    BaseLexicalParser() = default;
    BaseLexicalParser(vector<DFA> dfas);

    Token         parseStep(istream &input);
    vector<Token> parseAll(istream &input);
};

class LexicalParser : public BaseLexicalParser {
  private:
    map<int, int>    toSyntaxId_; // {x, syntaxId}
    map<int, string> tokenNames_; // {syntaxId,  name}

  public:
    LexicalParser(Grammar grammar, map<string, string> nameToRegex);
    Token         parseStep(istream &input);
    vector<Token> parseAll(istream &input);
};

} // namespace krill::runtime
#endif