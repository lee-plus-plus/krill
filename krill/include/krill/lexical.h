#ifndef LEXICAL_H
#define LEXICAL_H
#include "automata.h"
#include "defs.h"
#include "grammar.h"
#include <istream>
#include <string>
#include <vector>
using krill::type::Token, krill::type::DFA;
using std::vector, std::string, std::istream;

namespace krill::type {

struct Token {
    int    id;
    string lval;

    int row_st, col_st;
    int row_ed, col_ed;

    // to make std::set happy
    bool operator<(const Token &t) const;
    bool operator==(const Token &t) const;
    bool operator!=(const Token &t) const;
};

} // namespace krill::type

namespace krill::runtime {

class Lexer {
  protected:
    const DFA dfa_;

    int    state_;   // dfa state
    string history_; // input history

    int row_curr_, col_curr_;

    void count(const string &word);

  public:
    Lexer(DFA dfai);
    Lexer(vector<DFA> dfas);
    Lexer(vector<string> regexs);

    Token         parseStep(istream &input);
    vector<Token> parseAll(istream &input);
    void           clear();
};

} // namespace krill::runtime
#endif