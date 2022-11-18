#ifndef LEXICAL_H
#define LEXICAL_H
#include "defs.h"
#include "automata.h"
#include "grammar.h"
#include <istream>
#include <string>
#include <vector>
using krill::type::Token, krill::type::DFA;
using std::vector, std::string, std::istream;

namespace krill::type {

struct Token {
    // lexical token
    int    id;
    string lval;
    // to make std::set happy
    bool operator<(const Token &t) const;
    bool operator==(const Token &t) const;
    bool operator!=(const Token &t) const;
};

} // namespace krill::type

namespace krill::runtime {

class LexicalParser {
  public:
    LexicalParser() = default;
    LexicalParser(DFA dfai);
    LexicalParser(vector<DFA> dfas);
    LexicalParser(vector<string> regexs);

    Token         parseStep(istream &input);
    vector<Token> parseAll(istream &input);

  protected:
    DFA    dfa_;
    int    state_;
    string history_;
};

} // namespace krill::runtime
#endif