#ifndef SYNTAX_H
#define SYNTAX_H
#include "defs.h"
#include "grammar.h"
#include "attrdict.h"
#include <memory>
#include <ostream>
#include <stack>
#include <deque>
#include <string>
#include <vector>
using krill::type::Grammar, krill::type::ActionTable;
using krill::type::Token, krill::type::APTnode;
using krill::utils::AttrDict;
using std::vector, std::deque, std::stack;
using std::string, std::ostream;
using std::shared_ptr;

namespace krill::type {

// Annotated Parsing Tree Node
struct APTnode {
    int                        id;
    AttrDict                   attr;
    deque<shared_ptr<APTnode>> child;
};

} // namepspace krill::type

namespace krill::runtime {

using Afunc = std::function<void(AttrDict &next, AttrDict &child)>;
using Rfunc = std::function<void(AttrDict &next, deque<AttrDict> &child)>;

void defaultReduceFunc(AttrDict &next, deque<AttrDict> &child);
void defaultActionFunc(AttrDict &next, AttrDict &child);

class SyntaxParser {
  public:
    Afunc         actionFunc_; // binded with default function
    vector<Rfunc> reduceFunc_; // binded with default function

    SyntaxParser() = default;
    SyntaxParser(Grammar grammar, ActionTable actionTable);
    SyntaxParser(Grammar grammar, ActionTable actionTable, Afunc actionFunc, vector<Rfunc> reduceFunc);

    void clear();
    void parseStep(Token token);
    void parseStep(APTnode tokenWithAttr);
    void parseAll(vector<Token> tokens);
    void parseAll(vector<APTnode> tokensWithAttr);

    APTnode *getAnnotatedParsingTree();
    void printAnnotatedParsingTree(ostream &oss);

  private:
    Grammar       grammar_;
    ActionTable   actionTable_;

    vector<APTnode>  inputs_;
    stack<int>       states_;
    stack<shared_ptr<APTnode>> nodes_;

    int  offset_;
    bool isAccepted_;
    string history_;

    void parse();
    string getErrorMessage();
};

} // namespace krill::runtime
#endif