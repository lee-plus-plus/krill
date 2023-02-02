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

void AstPrinter::print_(const AstNode *const node, vector<bool> isLast,
                        ostream &oss) {
    if (skipMidNodes_ && node->child.size() == 1) {
        print_(node->child[0].get(), isLast, oss);
        return;
    }
    if (isLast.size() == 0) { oss << fmt::format(" "); }

    for (int i = 0; i < isLast.size(); i++) {
        if (i + 1 == isLast.size()) {
            oss << fmt::format("{:s}", (isLast[i] ? " └ " : " ├ "));
        } else {
            oss << fmt::format("{:s}", (isLast[i] ? "  " : " │"));
        }
    }
    string attr_str = showAttrs_ ? node->attr.str2() : "";
    oss << fmt::format("{:16s}  {}\n", fmt::format("<{:s}>", node->symname),
                       node->lval, attr_str);

    isLast.push_back(false);
    for (auto it = node->child.begin(); it != node->child.end(); it++) {
        if (it + 1 == node->child.end()) { isLast.back() = true; }
        print_((*it).get(), isLast, oss);
    }
}

string AstPrinter::print(const AstNode *const root) {
    stringstream ss;
    print_(root, {}, ss);
    return ss.str();
}

string AstPrinter::print(const vector<shared_ptr<AstNode>> &nodes) {
    stringstream ss;
    for (auto node : nodes) { print_(node.get(), {}, ss); }
    return ss.str();
}

// ---------- AstNode ----------

AstNode toAstNode(const Token &token, const map<int, string> &symbolNames) {
    return AstNode{.id      = token.id,
                   .pidx    = -1,
                   .symname = symbolNames.at(token.id),
                   .lval    = token.lval,
                   .attr    = token.attr,
                   .child   = {}};
}

// ---------- SyntaxParser ----------

SyntaxParser::SyntaxParser(const Grammar &    grammar,
                           const ActionTable &actionTable)
    : grammar_(grammar), actionTable_(actionTable), states_({0}), offset_(0),
      isAccepted_(false) {}

void SyntaxParser::parse() {
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
        }

        assert(actionTable_.count({states_.top(), input.id}) != 0);
        Action action = actionTable_.at({states_.top(), input.id});

        switch (action.type) {
        case Action::Type::kAction: {
            logger.debug("  ACTION push s{}", action.tgt);
            states_.push(action.tgt);
            symbols_.push(input.id);

            shared_ptr<AstNode> nextNode(
                new AstNode(toAstNode(input, symNames)));
            nextNode.get()->pidx = -1;
            // ACTION action
            onAction(*nextNode.get());

            nodes_.push(nextNode);

            offset_++;
            break;
        }
        case Action::Type::kReduce: {
            Prod                       r = prods_.at(action.tgt);
            deque<shared_ptr<AstNode>> childNodes;

            auto poped_states = get_top(states_, r.right.size());
            assert(nodes_.size() >= r.right.size());
            for (int j = 0; (int) j < r.right.size(); j++) {
                states_.pop();
                symbols_.pop();

                childNodes.push_front(nodes_.top());
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

            shared_ptr<AstNode> nextNode(
                new AstNode({.id      = r.symbol,
                             .pidx    = action.tgt,
                             .symname = symNames.at(r.symbol),
                             .lval    = "",
                             .attr    = {},
                             .child   = childNodes}));
            // REDUCE action
            onReduce(*nextNode.get());

            nodes_.push(nextNode);
            break;
        }
        case Action::Type::kAccept: {
            logger.debug("  ACCEPT");
            logger.info("syntax parsing complete successfully");
            logger.debug("Current AST: \n{}",
                         AstPrinter{}.print(to_vector(nodes_)));
            isAccepted_ = true;

            // ACCEPT action
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

void SyntaxParser::clear() {
    inputs_.clear();
    states_     = stack<int>();
    nodes_      = stack<shared_ptr<AstNode>>();
    offset_     = 0;
    isAccepted_ = false;
    states_.push(0);
}

void SyntaxParser::parseStep(Token token) {
    inputs_.push_back(token);
    parse();
}

// void SyntaxParser::parseStep(AstNode tokenWithAttr) {
//     inputs_.push_back(tokenWithAttr);
//     parse();
// }

void SyntaxParser::parseAll(vector<Token> tokens) {
    inputs_.insert(inputs_.end(), tokens.begin(), tokens.end());
    parse();
}

// void SyntaxParser::parseAll(vector<AstNode> tokensWithAttr) {
//     inputs_.insert(inputs_.end(), tokensWithAttr.begin(),
//     tokensWithAttr.end()); parse();
// }

shared_ptr<AstNode> SyntaxParser::getAstRoot() {
    if (!isAccepted_) {
        assert(false);
        return {nullptr};
    }
    assert(nodes_.size() == 1);

    return nodes_.top();
}

// ---------- DIY ----------

void SyntaxParser::onReduce(AstNode &node) {
    // pass
}

void SyntaxParser::onAction(AstNode &node) {
    // pass
}

void SyntaxParser::onAccept() {
    // pass
}

void SyntaxParser::onError() {
    auto & tokenWithAttr = inputs_.at(offset_);
    string errorMsg =
        fmt::format("syntax parsing error: unexpected {}",
                    tokenWithAttr.id == END_SYMBOL
                        ? "end of input"
                        : fmt::format(" <token {}> ‘{}’",
                                      grammar_.symbolNames.at(tokenWithAttr.id),
                                      unescape(tokenWithAttr.lval)));

    logger.debug("{}", errorMsg);
    logger.debug("  look: {} ‘{}’", grammar_.symbolNames.at(tokenWithAttr.id),
                 unescape(tokenWithAttr.attr.Get<string>("lval")));
    logger.debug(
        "  symbols: [{}]",
        fmt::join(apply_map(to_vector(symbols_), grammar_.symbolNames), ","));
    logger.debug("  states: [{}]", fmt::join(to_vector(states_), ","));
    logger.debug("Current APT: \n{}", AstPrinter{}.print(to_vector(nodes_)));

    throw runtime_error(errorMsg);
}

} // namespace krill::runtime