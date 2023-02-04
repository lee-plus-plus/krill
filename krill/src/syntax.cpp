#include "krill/syntax.h"
#include "fmt/color.h"
#include "fmt/format.h"
#include "krill/grammar.h"
#include "krill/lexical.h"
#include "krill/utils.h"
#include <cassert>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>
using krill::error::parse_error;
using krill::log::logger;
using namespace krill::type;
using namespace krill::grammar;
using namespace krill::utils;
using namespace std;

namespace krill::runtime {

// ------------------------------ AstPrinter ------------------------------

void AstPrinter::printElem(const AstNode *const node, ostream &oss) {
    oss << fmt::format(
        "{:16s}",
        fmt::format(
            "<{:s}>{}", // <type_spec>[1:13-1:16]
            node->symname,
            showLocation_
                ? fmt::format("{:13s}", fmt::format("[{}:{}-{}:{}]",
                                                    node->row_st, node->col_st,
                                                    node->row_ed, node->col_ed))
                : ""));

    if (showLexValue_) { oss << fmt::format("  {:6s}", node->lval); }
    if (showAttr_) { oss << fmt::format("  {:s}", node->attr.str2()); }
}

void AstPrinter::printTree(const AstNode *const node, vector<bool> isLast,
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

string AstPrinter::print(const AstNode *const root) {
    stringstream ss;
    printTree(root, {}, ss);
    return ss.str();
}

AstPrinter &AstPrinter::showLexValue(bool flag) {
    showLexValue_ = flag;
    return *this;
}


AstPrinter &AstPrinter::showMidNodes(bool flag) {
    showMidNodes_ = flag;
    return *this;
}

AstPrinter &AstPrinter::showLocation(bool flag) {
    showLocation_ = flag;
    return *this;
}

AstPrinter &AstPrinter::showAttr(bool flag) {
    showAttr_ = flag;
    return *this;
}

AstPrinter &AstPrinter::showColor(bool flag) {
    showColor_ = flag;
    return *this;
}

AstPrinter &AstPrinter::setWidth(int width) {
    assert(width >= 0);
    width_ = width;
    return *this;
}

// --------------------------------- Parser  ----------------------------------


void Parser::parse() { parse_impl(); }

void Parser::clear() { clear_impl(); }

void Parser::parseStep(const Token &token) {
    inputs_.push(token);
    parse();
}

void Parser::parseAll(const vector<Token> &tokens) {
    for (const auto &token : tokens) { inputs_.push(token); }
    parse();
}

bool Parser::isAccepted() const { return isAccepted_; }

shared_ptr<AstNode> Parser::getAstRoot() const {
    assert(isAccepted_);
    assert(nodes_.size() == 1);
    return nodes_.top();
}

Parser::Parser(const Grammar &grammar, const ActionTable &actionTable)
    : Parser_impl(grammar, actionTable){};

// ------------------------------- Parser_impl  -------------------------------

AstNode Parser_impl::toNode(const Token &token) {
    row_curr = token.row_ed;
    col_curr = token.col_ed;
    return AstNode{.id      = token.id,
                   .pidx    = -1,
                   .child   = {},
                   .symname = grammar_.symbolNames.at(token.id),
                   .lval    = token.lval,
                   .row_st  = token.row_st,
                   .col_st  = token.col_st,
                   .row_ed  = token.row_ed,
                   .col_ed  = token.col_ed};
}

AstNode Parser_impl::toNode(const deque<shared_ptr<AstNode>> &child, int id,
                            int pidx) {
    int row_st = child.size() ? child.front().get()->row_st : row_curr;
    int col_st = child.size() ? child.front().get()->col_st : col_curr;
    int row_ed = child.size() ? child.back().get()->row_ed : row_curr;
    int col_ed = child.size() ? child.back().get()->col_ed : col_curr;

    row_curr = row_ed;
    col_curr = col_ed;

    return AstNode{.id      = id,
                   .pidx    = pidx,
                   .child   = child,
                   .symname = grammar_.symbolNames.at(id),
                   .lval    = "",
                   .row_st  = row_st,
                   .col_st  = col_st,
                   .row_ed  = row_ed,
                   .col_ed  = col_ed};
}

void Parser_impl::parse_impl() {
    const auto &prods_   = grammar_.prods;
    const auto &symNames = grammar_.symbolNames;

    while (!isAccepted_ && inputs_.size()) {
        Token input = inputs_.front();

        logger.debug("parser: states_: [{}]",
                     fmt::join(to_vector(states_), ","));
        logger.debug("parser: symbols_: [{}]",
                     fmt::join(to_vector(symbols_), ","));
        logger.debug("  parser: look: <token {}> ‘{}’", symNames.at(input.id),
                     unescape(input.lval));

        assert(states_.size() > 0);
        if (actionTable_.count({states_.top(), input.id}) == 0) {
            logger.debug("  parser: ERROR");
            // DIY onError();
            onError();
            throw parse_error(
                input.row_st, input.col_st,
                fmt::format("meet unexpected {}",
                            input.id == END_SYMBOL
                                ? "end of input"
                                : fmt::format(" <token {}> ‘{}’",
                                              symNames.at(input.id),
                                              unescape(input.lval))));
        }

        assert(actionTable_.count({states_.top(), input.id}) != 0);
        Action action = actionTable_.at({states_.top(), input.id});

        switch (action.type) {
        case Action::Type::kAction: {
            logger.debug("  parser: ACTION push [{}]", action.tgt);
            states_.push(action.tgt);
            symbols_.push(symNames.at(input.id));

            auto nextNode = make_shared<AstNode>(toNode(input));
            // DIY onAction(AstNode *node)
            onAction(nextNode.get());
            nodes_.push(nextNode);
            inputs_.pop();
            break;
        }
        case Action::Type::kReduce: {
            const Prod &               r = prods_.at(action.tgt);
            deque<shared_ptr<AstNode>> child;

            auto poped_states = get_top(states_, r.right.size());
            for (int j = 0; (int) j < r.right.size(); j++) {
                states_.pop();
                symbols_.pop();
                child.push_front(nodes_.top());
                nodes_.pop();
            }

            assert(actionTable_.count({states_.top(), r.symbol}) != 0);
            Action action2 = actionTable_.at({states_.top(), r.symbol});
            assert(action2.type == Action::Type::kGoto);
            states_.push(action2.tgt);
            symbols_.push(symNames.at(r.symbol));
            assert(0 <= action.tgt && action.tgt < prods_.size());

            logger.debug("  parser: REDUCE r{} pop [{}] GOTO {}", action.tgt,
                         fmt::join(poped_states, ","), action2.tgt);

            auto nextNode =
                make_shared<AstNode>(toNode(child, r.symbol, action.tgt));
            // DIY onReduce(AstNode *node)
            onReduce(nextNode.get());
            nodes_.push(nextNode);
            break;
        }
        case Action::Type::kAccept: {
            logger.debug("  parser: ACCEPT");
            isAccepted_ = true;

            // DIY onAccept()
            onAccept();

            break;
        }
        default: {
            assert(false);
            break;
        }
        }
    }
}

void Parser_impl::clear_impl() {
    inputs_     = queue<Token>{};
    states_     = stack<int>();
    symbols_    = stack<string>();
    nodes_      = stack<shared_ptr<AstNode>>();
    row_curr    = 1;
    col_curr    = 1;
    isAccepted_ = false;
    states_.push(0);
}

Parser_impl::Parser_impl(const Grammar &grammar, const ActionTable &actionTable)
    : grammar_(grammar), actionTable_(actionTable), inputs_(), states_({0}),
      symbols_(), nodes_(), row_curr(1), col_curr(1), isAccepted_(false) {}


} // namespace krill::runtime