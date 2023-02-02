#ifndef LEXICAL_H
#define LEXICAL_H
#include "attrdict.h"
#include "automata.h"
#include "defs.h"
#include "grammar.h"
#include <istream>
#include <string>
#include <vector>
using krill::type::Token, krill::type::DFA;
using krill::utils::AttrDict;
using std::vector, std::string, std::istream;

namespace krill::type {

// lexical token
struct Token {
    int      id;
    string   lval;
    AttrDict attr;
    // to make std::set happy
    bool operator<(const Token &t) const;
    bool operator==(const Token &t) const;
    bool operator!=(const Token &t) const;
};

} // namespace krill::type

namespace krill::runtime {

class LexicalParser {
  protected:
    const DFA dfa_;

    int       state_;   // dfa state
    string    history_; // input history 

  public:
    // LexicalParser() = default;
    LexicalParser(DFA dfai);
    LexicalParser(vector<DFA> dfas);
    LexicalParser(vector<string> regexs);

    Token         parseStep(istream &input);
    vector<Token> parseAll(istream &input);
    void          clear();
};

} // namespace krill::runtime
#endif