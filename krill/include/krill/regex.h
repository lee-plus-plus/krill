#ifndef REGEX_H
#define REGEX_H
#include "automata.h"
#include "defs.h"
#include <map>
#include <stack>
#include <string>
#include <vector>
using krill::automata::DFA, krill::automata::NFA, krill::automata::EdgeTable;
using krill::grammar::Token, krill::grammar::Grammar,
    krill::grammar::ActionTable;
using std::string, std::pair, std::vector, std::map, std::stack;

namespace krill::regex {

DFA getDFAfromRegex(string src);
NFA getNFAfromRegex(string src);

} // namespace krill::regex

namespace krill::regex::core {

struct Token {
    int    id;
    string lexValue;
    char   realValue;
    int    st;
    int    ed;
};

struct Node {
    int       id;
    string    lexValue;
    char      realValue;
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