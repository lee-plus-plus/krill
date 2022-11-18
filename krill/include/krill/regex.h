// poorly written, but it can run
// maybe re-write later
#ifndef REGEX_H
#define REGEX_H
#include "automata.h"
#include "defs.h"
#include <map>
#include <stack>
#include <string>
#include <vector>
using krill::type::DFA, krill::type::NFA, krill::type::EdgeTable;
using krill::type::Token, krill::type::Grammar, krill::type::ActionTable;
using std::string, std::pair, std::vector, std::map, std::stack;

namespace krill::regex {

DFA getDFAfromRegex(string src);
NFA getNFAfromRegex(string src);

} // namespace krill::regex

namespace krill::regex::core {

// you don't use these
struct Token {
    int    id;
    string lval;
    char   rval;
    int    st;
    int    ed;
};

struct Node {
    int       id;
    string    lval;
    char      rval;
    int       nfaSt, nfaEd;
    set<char> rangeChars;
    Node *    child;
    int       st, ed;
};

class RegexParser {
  protected:
    static const Grammar     grammar_;
    static const ActionTable actionTable_;

    string        regex_;
    stack<int>    states_;
    vector<Token> tokens_;
    stack<Node>   nodes_;

    EdgeTable nfaEdges;    // {{symbol, from, to}}
    int       numNfaNodes; // leave 0 for global start, 1 for global end

    int  posTokens_;
    bool isAccepted_;

    void lexicalParse();
    void syntaxParse();

  public:
    RegexParser(string regex);

    // bool match(string src);
    NFA nfa();
    DFA dfa();
};

} // namespace krill::regex::core
#endif