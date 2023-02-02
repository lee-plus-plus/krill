#ifndef SYNTAX_H
#define SYNTAX_H
#include "defs.h"
#include "grammar.h"
#include <deque>
#include <memory>
#include <ostream>
#include <stack>
#include <string>
#include <vector>
using krill::type::Grammar, krill::type::ActionTable;
using krill::type::Token;
using std::shared_ptr;
using std::string, std::ostream;
using std::vector, std::deque, std::stack;

namespace krill::type {

// Annotated Parsing Tree Node
struct AstNode {
    int                        id;   // syntax id
    int                        pidx; // production id, -1 if not from reducution
    deque<shared_ptr<AstNode>> child;

    string symname; // grammar symbol name
    string lval;    // lexical value
    int    row_st, col_st;
    int    row_ed, col_ed;
};

} // namespace krill::type

namespace krill::runtime {

using krill::type::AstNode;

class AstPrinter {
  private:
    bool showColor_    = false;
    bool showAttrs_    = false;
    bool skipMidNodes_ = true;
    int  width_        = 1;

    void printElem(const AstNode *const node, ostream &oss);
    void printTree(const AstNode *const node, vector<bool> isLast,
                   ostream &oss);

  public:
    AstPrinter() = default;

    AstPrinter &showColor(bool flag = true);
    AstPrinter &showAttrs(bool flag = true);
    AstPrinter &skipMidNodes(bool flag = true);
    AstPrinter &setWidth(int width);

    string print(const AstNode *const root);
    string print(const vector<shared_ptr<AstNode>> &nodes);
};


// syntax parser
class Parser {
  private:
    const Grammar     grammar_;
    const ActionTable actionTable_;

    vector<Token>              inputs_;  // input
    stack<int>                 states_;  // lr(1) states
    stack<int>                 symbols_; // lr(1) symbols (for debug)
    stack<shared_ptr<AstNode>> nodes_;   // parsed result

    int row_curr, col_curr; // help locate those non-child node

    int  offset_;
    bool isAccepted_;

    AstNode toNode(const Token &token);
    AstNode toNode(const deque<shared_ptr<AstNode>> &child, int id, int pidx);

    void parse();

    // DIY
    void onAction(AstNode *node);
    void onReduce(AstNode *node);
    void onAccept();
    void onError();

  public:
    Parser(const Grammar &grammar, const ActionTable &actionTable);

    void clear();
    void parseStep(Token token);
    void parseAll(vector<Token> tokens);

    bool isAccepted() const;

    shared_ptr<AstNode> getAstRoot();
};

} // namespace krill::runtime
#endif
