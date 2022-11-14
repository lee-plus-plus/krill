#ifndef SYNTAX_H
#define SYNTAX_H
#include "defs.h"
#include "grammar.h"
#include <map>
#include <ostream>
#include <set>
#include <string>
#include <vector>
using krill::grammar::Grammar, krill::grammar::ActionTable,
    krill::grammar::Token;
using std::map, std::vector;
using std::string, std::ostream;

namespace krill::runtime {

// Annotated Parsing Tree Node
struct APTnode {
    int               syntaxId;
    string            lexValue;
    vector<APTnode *> child;
};

class SyntaxParser {
  private:
    Grammar     grammar_;
    ActionTable actionTable_;

    vector<Token>     tokens_;
    vector<int>       states_;
    vector<APTnode *> nodes_;

    int  posTokens_;
    bool isAccepted_;

    void parse();

  public:
    SyntaxParser() = default;
    SyntaxParser(Grammar grammar, ActionTable actionTable);

    void reset();
    void parseStep(Token token);
    void parseAll(vector<Token> tokens);

    APTnode *getAnnotatedParsingTree();
    void     printAnnotatedParsingTree(ostream &oss);
};

} // namespace krill::runtime
#endif