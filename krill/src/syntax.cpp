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

// ---------- AstPrinter ----------

AstPrinter &AstPrinter::showColor(bool flag) {
    showColor_ = flag;
    return *this;
}

AstPrinter &AstPrinter::showAttrs(bool flag) {
    showAttrs_ = flag;
    return *this;
}

AstPrinter &AstPrinter::skipMidNodes(bool flag) {
    skipMidNodes_ = flag;
    return *this;
}

AstPrinter &AstPrinter::setWidth(int width) {
    assert(width >= 0);
    width_ = width;
    return *this;
}

void AstPrinter::printElem(const AstNode *const node, ostream &oss) {
    oss << fmt::format("{:16s}  {}\n", fmt::format("<{:s}>", node->symname),
                       node->lval);
}

void AstPrinter::printTree(const AstNode *const node, vector<bool> isLast,
                           ostream &oss) {
    if (skipMidNodes_ && node->child.size() == 1) {
        printTree(node->child[0].get(), isLast, oss);
        return;
    }
    if (isLast.size() == 0) { oss << fmt::format(" "); }

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

string AstPrinter::print(const vector<shared_ptr<AstNode>> &nodes) {
    stringstream ss;
    for (auto node : nodes) { printTree(node.get(), {}, ss); }
    return ss.str();
}

// ---------- Parser private ----------

AstNode Parser::toNode(const Token &token) {
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

AstNode Parser::toNode(const deque<shared_ptr<AstNode>> &child, int id,
                       int pidx) {
    int row_st, col_st;
    int row_ed, col_ed;

    if (child.size() > 0) {
        row_st = child.front().get()->row_st;
        col_st = child.front().get()->col_st;
        row_ed = child.back().get()->row_ed;
        col_ed = child.back().get()->col_ed;
    } else {
        row_st = row_curr;
        col_st = col_curr;
        row_ed = row_curr;
        col_ed = col_curr;
    }

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

void Parser::parse() {
    const auto &prods_   = grammar_.prods;
    const auto &symNames = grammar_.symbolNames;

    while (!isAccepted_ && offset_ < inputs_.size()) {
        Token &input = inputs_.at(offset_);
        // assert(input.attr.Has<string>("lval"));

        logger.debug("states_: [{}]", fmt::join(to_vector(states_), ","));
        logger.debug("symbols_: [{}]",
                     fmt::join(apply_map(to_vector(symbols_), symNames), ","));
        logger.debug("  look: {}", symNames.at(input.id));

        assert(states_.size() > 0);
        if (actionTable_.count({states_.top(), input.id}) == 0) {
            // ERROR action
            onError();
            throw parse_error(
                input.row_st, input.col_st,
                fmt::format("syntax parsing meet unexpected {}",
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
            logger.debug("  ACTION push s{}", action.tgt);
            states_.push(action.tgt);
            symbols_.push(input.id);

            // ACTION action
            auto nextNode = make_shared<AstNode>(toNode(input));
            onAction(nextNode.get());

            nodes_.push(nextNode);

            offset_++;
            break;
        }
        case Action::Type::kReduce: {
            Prod                       r = prods_.at(action.tgt);
            deque<shared_ptr<AstNode>> child;

            auto poped_states = get_top(states_, r.right.size());
            assert(nodes_.size() >= r.right.size());
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
            symbols_.push(r.symbol);
            assert(0 <= action.tgt && action.tgt < prods_.size());

            logger.debug("  REDUCE r{} pop [{}] GOTO {}", action.tgt + 1,
                         fmt::join(poped_states, ","), action2.tgt);

            // REDUCE action
            auto nextNode =
                make_shared<AstNode>(toNode(child, r.symbol, action.tgt));
            onReduce(nextNode.get());

            nodes_.push(nextNode);
            break;
        }
        case Action::Type::kAccept: {
            logger.debug("  ACCEPT");
            logger.info("syntax parsing complete successfully");
            logger.debug("Current AST: \n{}",
                         AstPrinter{}.print(to_vector(nodes_)));

            // ACCEPT action
            isAccepted_ = true;
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

// ---------- Parser public ----------

Parser::Parser(const Grammar &grammar, const ActionTable &actionTable)
    : grammar_(grammar), actionTable_(actionTable), inputs_({}), states_({0}),
      symbols_(), nodes_(), row_curr(1), col_curr(1), offset_(0),
      isAccepted_(false) {}

void Parser::clear() {
    inputs_     = vector<Token>{};
    states_     = stack<int>();
    symbols_    = stack<int>();
    nodes_      = stack<shared_ptr<AstNode>>();
    row_curr    = 1;
    col_curr    = 1;
    offset_     = 0;
    isAccepted_ = false;
    states_.push(0);
}

void Parser::parseStep(Token token) {
    inputs_.push_back(token);
    parse();
}

void Parser::parseAll(vector<Token> tokens) {
    inputs_.insert(inputs_.end(), tokens.begin(), tokens.end());
    parse();
}

shared_ptr<AstNode> Parser::getAstRoot() {
    if (!isAccepted_) {
        assert(false);
        return {nullptr};
    }
    assert(nodes_.size() == 1);
    return nodes_.top();
}

bool Parser::isAccepted() const { return isAccepted_; }

// ---------- DIY ----------

void Parser::onReduce(AstNode *node) {}

void Parser::onAction(AstNode *node) {}

void Parser::onAccept() {}

void Parser::onError() {
    auto &token = inputs_.at(offset_);

    logger.debug(
        "{}", fmt::format("syntax parsing error: unexpected {}",
                          token.id == END_SYMBOL
                              ? "end of input"
                              : fmt::format(" <token {}> ‘{}’",
                                            grammar_.symbolNames.at(token.id),
                                            unescape(token.lval))));
    logger.debug("  look: {} ‘{}’", grammar_.symbolNames.at(token.id),
                 unescape(token.lval));
    logger.debug(
        "  symbols: [{}]",
        fmt::join(apply_map(to_vector(symbols_), grammar_.symbolNames), ","));
    logger.debug("  states: [{}]", fmt::join(to_vector(states_), ","));
    logger.debug("Current APT: \n{}", AstPrinter{}.print(to_vector(nodes_)));
}

} // namespace krill::runtime