#ifndef SYNTAX_H
#define SYNTAX_H
// #include "attrdict.h"
#include "defs.h"
#include "grammar.h"
#include "lexical.h"
#include "utils.h"
#include <deque>
#include <memory>
#include <ostream>
#include <sstream>
#include <stack>
#include <queue>
#include <string>
#include <vector>
// using krill::type::AttrDict;
using krill::type::Prod;
using krill::type::Grammar, krill::type::ActionTable;
using krill::type::Token;
using krill::utils::is_template_of;
using std::shared_ptr, std::make_shared;
using std::string, std::ostream, std::stringstream;
using std::vector, std::deque, std::stack,std::queue;

namespace krill::type {

// Annotated Parsing Tree Node
// can be extended by templating attribute
template <typename AttrT>
struct BaseAstNode {
    int id;   // syntax id
    int pidx; // production id, -1 if not from reducution

    deque<shared_ptr<BaseAstNode<AttrT>>> child;

    string symname; // grammar symbol name
    string lval;    // lexical value
    int    row_st, col_st;
    int    row_ed, col_ed;

    AttrT attr; // DIY
};

} // namespace krill::type

namespace krill::runtime {

// --------------------------- AstPrinter definition --------------------------

using krill::type::Token;
using krill::type::BaseAstNode;

class AstPrinter {
  private:
    bool showMidNodes_ = false;
    bool showLocation_ = false;
    bool showColor_    = false;
    int  width_        = 1;

    template <typename AstNodeT>
    void printElem(const AstNodeT *const node, ostream &oss);

    template <typename AstNodeT>
    void printTree(const AstNodeT *const node, vector<bool> isLast,
                   ostream &oss);

  public:
    AstPrinter() = default;

    AstPrinter &showMidNodes(bool flag = true);
    AstPrinter &showLocation(bool flag = true);
    AstPrinter &showColor(bool flag = true);
    AstPrinter &setWidth(int width);

    template <typename AstNodeT>
    string print(const AstNodeT *const root);
};

// ------------------------- AstPrinter implementation ------------------------

template <typename AstNodeT>
void AstPrinter::printElem(const AstNodeT *const node, ostream &oss) {
    oss << fmt::format("{:16s}  {:6s}", fmt::format("<{:s}>", node->symname),
                       node->lval);
}

template <typename AstNodeT>
void AstPrinter::printTree(const AstNodeT *const node, vector<bool> isLast,
                           ostream &oss) {
    if (showMidNodes_ && node->child.size() == 1) {
        printTree(node->child[0].get(), isLast, oss);
        return;
    }
    if (isLast.size() == 0) { oss << " "; }

    for (int i = 0; i < isLast.size(); i++) {
        if (i + 1 == isLast.size()) {
            oss << fmt::format("{}", (isLast[i] ? " └" : " ├"));
            for (int j = 0; j < width_; j++) { oss << "─"; }
            oss << " ";
        } else {
            oss << fmt::format("{}", (isLast[i] ? "  " : " │"));
            for (int j = 0; j < width_; j++) { oss << " "; }
        }
    }

    printElem(node, oss);
    oss << "\n";

    isLast.push_back(false);
    for (auto it = node->child.begin(); it != node->child.end(); it++) {
        if (it + 1 == node->child.end()) { isLast.back() = true; }
        printTree((*it).get(), isLast, oss);
    }
}

template <typename AstNodeT>
string AstPrinter::print(const AstNodeT *const root) {
    static_assert(is_template_of<BaseAstNode, AstNodeT>::value);
    stringstream ss;
    printTree(root, {}, ss);
    return ss.str();
}


// --------------------------- BaseParser definition --------------------------

// ignore this
class BaseParser_core {
  protected:
    const Grammar     grammar_;
    const ActionTable actionTable_;

    queue<Token>  inputs_;  // input
    stack<int>    states_;  // lr(1) states
    stack<string> symbols_; // lr(1) symbols (for debug)

    int row_curr, col_curr; // help locate those non-child node

    int  offset_;
    bool isAccepted_;

    void parse();
    void clear_core();

    virtual void onAction_impl(Token token) = 0;
    virtual void onReduce_impl(int pidx) = 0;
    virtual void onAccept_impl() = 0;
    virtual void onError_impl() = 0;

  public:
    BaseParser_core(const Grammar &grammar, const ActionTable &actionTable);
};

// ignore this
template <typename AstNodeT>
class BaseParser_impl : public BaseParser_core {
  protected:
    using AstNodePtrT = shared_ptr<AstNodeT>;
    stack<AstNodePtrT> nodes_; // parsed result

    void onAction_impl(Token token);
    void onReduce_impl(int pidx);
    void onAccept_impl();
    void onError_impl();

    AstNodeT toNode(const Token &token);
    AstNodeT toNode(const deque<shared_ptr<AstNodeT>> &child, int id, int pidx);

    virtual void onAction(AstNodeT *node) = 0;
    virtual void onReduce(AstNodeT *node) = 0;
    virtual void onAccept() = 0;
    virtual void onError() = 0;

  public:
    BaseParser_impl(const Grammar &grammar, const ActionTable &actionTable);
};

// use this
template <typename AstNodeT>
class BaseParser : public BaseParser_impl<AstNodeT> {
  protected:
    using Base = BaseParser_impl<AstNodeT>;

    void onAction(AstNodeT *node); // DIY
    void onReduce(AstNodeT *node); // DIY
    void onAccept(); // DIY
    void onError(); // DIY

  public:
    void clear();
    void parseStep(Token token);
    void parseAll(vector<Token> tokens);

    bool                isAccepted() const;
    shared_ptr<AstNodeT> getAstRoot() const;

    BaseParser(const Grammar &grammar, const ActionTable &actionTable);
};

// --------------------------- default instance ----------------------------

// if you don't need such complex template to deal with attributes, balabala, 
// just use them!
using Attr_no = int;
using AstNode = BaseAstNode<Attr_no>;
using Parser  = BaseParser<AstNode>;

// ---------------------- BaseParser_impl implementation ----------------------

template <typename AstNodeT>
void BaseParser_impl<AstNodeT>::onAction_impl(Token token) {
    auto nextNode = make_shared<AstNodeT>(toNode(token));
    onAction(nextNode.get());
    nodes_.push(nextNode);
}

template <typename AstNodeT>
void BaseParser_impl<AstNodeT>::onReduce_impl(int pidx) {
    const Prod &                r = grammar_.prods.at(pidx);
    deque<AstNodePtrT> child;

    for (int j = 0; j < r.right.size(); j++) {
        child.push_front(nodes_.top());
        nodes_.pop();
    }

    // REDUCE action
    auto nextNode = make_shared<AstNodeT>(toNode(child, r.symbol, pidx));
    onReduce(nextNode.get());
    nodes_.push(nextNode);
}

template <typename AstNodeT>
void BaseParser_impl<AstNodeT>::onAccept_impl() {
    onAccept();
}

template <typename AstNodeT>
void BaseParser_impl<AstNodeT>::onError_impl() {
    onError();
}

template <typename AstNodeT>
AstNodeT BaseParser_impl<AstNodeT>::toNode(const Token &token) {
    row_curr = token.row_ed;
    col_curr = token.col_ed;
    return AstNodeT{.id      = token.id,
                   .pidx    = -1,
                   .child   = {},
                   .symname = grammar_.symbolNames.at(token.id),
                   .lval    = token.lval,
                   .row_st  = token.row_st,
                   .col_st  = token.col_st,
                   .row_ed  = token.row_ed,
                   .col_ed  = token.col_ed};
}

template <typename AstNodeT>
AstNodeT BaseParser_impl<AstNodeT>::toNode(const deque<AstNodePtrT> &child,
                                           int id, int pidx) {
    int row_st = child.size() ? child.front().get()->row_st : row_curr;
    int col_st = child.size() ? child.front().get()->col_st : col_curr;
    int row_ed = child.size() ? child.back().get()->row_ed : row_curr;
    int col_ed = child.size() ? child.back().get()->col_ed : col_curr;
    row_curr = row_ed;
    col_curr = col_ed;
    return AstNodeT{.id      = id,
                   .pidx    = pidx,
                   .child   = child,
                   .symname = grammar_.symbolNames.at(id),
                   .lval    = "",
                   .row_st  = row_st,
                   .col_st  = col_st,
                   .row_ed  = row_ed,
                   .col_ed  = col_ed};
}

template <typename AstNodeT>
BaseParser_impl<AstNodeT>::BaseParser_impl(const Grammar &    grammar,
                                           const ActionTable &actionTable)
    : BaseParser_core(grammar, actionTable){};

// ------------------------- BaseParser implementation ------------------------

// DIY
template <typename AstNodeT>
void BaseParser<AstNodeT>::onAction(AstNodeT *node) {}

// DIY
template <typename AstNodeT>
void BaseParser<AstNodeT>::onReduce(AstNodeT *node) {}

// DIY
template <typename AstNodeT>
void BaseParser<AstNodeT>::onAccept() {}

// DIY
template <typename AstNodeT>
void BaseParser<AstNodeT>::onError() {}

template <typename AstNodeT>
void BaseParser<AstNodeT>::clear() {
    Base::clear_core();
    Base::nodes_ = stack<shared_ptr<AstNodeT>>();
}

template <typename AstNodeT>
void BaseParser<AstNodeT>::parseStep(Token token) {
    Base::inputs_.push(token);
    Base::parse();
}

template <typename AstNodeT>
void BaseParser<AstNodeT>::parseAll(vector<Token> tokens) {
    for (const auto &token : tokens) { Base::inputs_.push(token); }
    Base::parse();
}

template <typename AstNodeT>
bool BaseParser<AstNodeT>::isAccepted() const {
    return Base::isAccepted_;
}

template <typename AstNodeT>
shared_ptr<AstNodeT> BaseParser<AstNodeT>::getAstRoot() const {
    assert(Base::isAccepted_);
    assert(Base::nodes_.size() == 1);
    return Base::nodes_.top();
}

template <typename AstNodeT>
BaseParser<AstNodeT>::BaseParser(const Grammar &grammar, const ActionTable &actionTable):
    BaseParser_impl<AstNodeT>(grammar, actionTable) {};

// ------------------------- BaseParser implementation ------------------------

} // namespace krill::runtime
#endif
