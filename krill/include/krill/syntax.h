#ifndef SYNTAX_H
#define SYNTAX_H
#include "attrdict.h"
#include "defs.h"
#include "grammar.h"
#include <deque>
#include <memory>
#include <ostream>
#include <stack>
#include <string>
#include <vector>
using krill::type::Grammar, krill::type::ActionTable;
using krill::type::Token, krill::type::APTnode;
using krill::utils::AttrDict;
using std::shared_ptr;
using std::string, std::ostream;
using std::vector, std::deque, std::stack;

namespace krill::type {

// Annotated Parsing Tree Node
struct APTnode {
    int                        id;
    int                        pidx;
    AttrDict                   attr;
    deque<shared_ptr<APTnode>> child;
};

} // namespace krill::type

namespace krill::runtime {

using AptNodeFunc = std::function<void(APTnode &node)>;
inline AptNodeFunc defaultAptNodeFunc = [](APTnode &node) {};

class SyntaxParser {
  public:
    AptNodeFunc actionFunc_ = defaultAptNodeFunc;
    AptNodeFunc reduceFunc_ = defaultAptNodeFunc;
    AptNodeFunc errorFunc_ = defaultAptNodeFunc;

    SyntaxParser() = default;
    SyntaxParser(Grammar grammar, ActionTable actionTable);

    void clear();
    void parseStep(Token token);
    void parseStep(APTnode tokenWithAttr);
    void parseAll(vector<Token> tokens);
    void parseAll(vector<APTnode> tokensWithAttr);

    shared_ptr<APTnode> getAPT();
    string getAPTstr();
    string getASTstr(); // simplified

  private:
    Grammar     grammar_;
    ActionTable actionTable_;

    vector<APTnode>            inputs_;
    stack<int>                 states_;
    stack<int>                 symbols_; // for debug
    stack<shared_ptr<APTnode>> nodes_;

    int    offset_;
    bool   isAccepted_;
    string history_;

    void   parse();
    string getErrorMessage();
};

string getAPTstr(const shared_ptr<APTnode> &root, const Grammar &grammar);
string getASTstr(const shared_ptr<APTnode> &root, const Grammar &grammar);

} // namespace krill::runtime
#endif