#ifndef SYNTAX_H
#define SYNTAX_H
// #include "attrdict.h"
#include "attrdict.h"
#include "defs.h"
#include "grammar.h"
#include "lexical.h"
#include <deque>
#include <memory>
#include <ostream>
#include <queue>
#include <sstream>
#include <stack>
#include <string>
#include <vector>
using krill::type::AttrDict;
using krill::type::Grammar, krill::type::ActionTable;
using krill::type::Prod;
using krill::type::Token;
using std::shared_ptr, std::make_shared;
using std::string, std::ostream, std::stringstream;
using std::vector, std::deque, std::stack, std::queue;

namespace krill::type {

// Annotated Parsing Tree Node
// can be extended by templating attribute
struct AstNode {
    int id;   // syntax id
    int pidx; // production id, -1 if not from reducution

    deque<shared_ptr<AstNode>> child;

    string symname; // grammar symbol name
    string lval;    // lexical value
    int    row_st, col_st;
    int    row_ed, col_ed;

    AttrDict attr; // DIY
};

} // namespace krill::type

namespace krill::runtime {

// --------------------------- AstPrinter definition --------------------------

using krill::type::Token;
using krill::type::AstNode;

class AstPrinter {
  private:
    bool showLexValue_ = true;
    bool showMidNodes_ = false;
    bool showLocation_ = false;
    bool showAttr_     = false;
    bool showColor_    = false;
    int  width_        = 1;

    void printElem(const AstNode *const node, ostream &oss);
    void printTree(const AstNode *const node, vector<bool> isLast,
                   ostream &oss);

  public:
    AstPrinter() = default;

    AstPrinter &showLexValue(bool flag = true);
    AstPrinter &showMidNodes(bool flag = true);
    AstPrinter &showLocation(bool flag = true);
    AstPrinter &showAttr(bool flag = true);
    AstPrinter &showColor(bool flag = true);
    AstPrinter &setWidth(int width);

    string print(const AstNode *const root);
};

// --------------------------- BaseParser definition --------------------------

class Parser_impl {
  protected:
    const Grammar     grammar_;
    const ActionTable actionTable_;

    queue<Token>               inputs_;  // input
    stack<int>                 states_;  // lr(1) states
    stack<string>              symbols_; // lr(1) symbols (for debug)
    stack<shared_ptr<AstNode>> nodes_;   // parsed result

    int  row_curr, col_curr; // help locate those non-child node
    bool isAccepted_;

    AstNode toNode(const Token &token);
    AstNode toNode(const deque<shared_ptr<AstNode>> &child, int id, int pidx);

    void parse_impl();
    void clear_impl();

    virtual void onAction(AstNode *node) = 0;
    virtual void onReduce(AstNode *node) = 0;
    virtual void onAccept()              = 0;
    virtual void onError()               = 0;

    Parser_impl(const Grammar &grammar, const ActionTable &actionTable);
};

class Parser : public Parser_impl {
  protected:
    void onAction(AstNode *node){}; // DIY
    void onReduce(AstNode *node){}; // DIY
    void onAccept(){};              // DIY
    void onError(){};               // DIY

    void parse();

  public:
    void clear();
    void parseStep(const Token &token);
    void parseAll(const vector<Token> &tokens);

    bool                isAccepted() const;
    shared_ptr<AstNode> getAstRoot() const;

    Parser(const Grammar &grammar, const ActionTable &actionTable);
};

} // namespace krill::runtime
#endif
