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

// const int STATES_SIZE_MAX = 1000;

SyntaxParser::SyntaxParser(Grammar grammar, ActionTable actionTable)
    : grammar_(grammar), actionTable_(actionTable), states_({0}), offset_(0),
      isAccepted_(false), history_("") {}

void SyntaxParser::parse() {
    auto &      prods_   = grammar_.prods;
    const auto &symNames = grammar_.symbolNames;

    while (!isAccepted_ && offset_ < inputs_.size()) {
        APTnode &input = inputs_.at(offset_);
        assert(input.attr.Has<string>("lval"));

        logger.debug("states_: [{}]", fmt::join(to_vector(states_), ","));
        logger.debug("symbols_: [{}]",
                     fmt::join(apply_map(to_vector(symbols_), symNames), ","));
        logger.debug("  look: {}", symNames.at(input.id));

        assert(states_.size() > 0);
        if (actionTable_.count({states_.top(), input.id}) == 0) {
            throw runtime_error(getErrorMessage());
        }

        assert(actionTable_.count({states_.top(), input.id}) != 0);
        Action action = actionTable_[{states_.top(), input.id}];

        switch (action.type) {
            case Action::Type::kAction: {
                logger.debug("  ACTION push s{}", action.tgt);
                states_.push(action.tgt);
                symbols_.push(input.id);

                shared_ptr<APTnode> nextNode(new APTnode(input));
                nextNode.get()->pidx = -1;
                // ACTION action
                nodes_.push(nextNode);

                // very stupid history stack, may be improved in the future
                history_ += " ";
                history_ += input.attr.Get<string>("lval");
                if (history_.size() > 50) {
                    history_ = history_.substr(history_.size() - 50);
                }

                offset_++;
                break;
            }
            case Action::Type::kReduce: {
                Prod                       r = prods_[action.tgt];
                deque<shared_ptr<APTnode>> childNodes;

                auto poped_states = get_top(states_, r.right.size());
                for (int j = 0; (int) j < r.right.size(); j++) {
                    states_.pop();
                    symbols_.pop();
                    childNodes.push_front(nodes_.top());
                    nodes_.pop();
                }

                assert(actionTable_.count({states_.top(), r.symbol}) != 0);
                Action action2 = actionTable_[{states_.top(), r.symbol}];
                assert(action2.type == Action::Type::kGoto);
                states_.push(action2.tgt);
                symbols_.push(r.symbol);
                assert(0 <= action.tgt && action.tgt < prods_.size());

                logger.debug("  REDUCE r{} pop [{}] GOTO {}", action.tgt + 1,
                             fmt::join(poped_states, ","), action2.tgt);

                shared_ptr<APTnode> nextNode(
                    new APTnode({.id    = r.symbol,
                                 .pidx  = action.tgt,
                                 .attr  = {},
                                 .child = childNodes}));
                // REDUCE action

                nodes_.push(nextNode);
                break;
            }
            case Action::Type::kAccept: {
                logger.debug("  ACCEPT");
                logger.info("syntax parsing complete successfully");
                logger.debug("Current AST: \n{}", getASTstr());
                isAccepted_ = true;
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
    nodes_      = stack<shared_ptr<APTnode>>();
    offset_     = 0;
    isAccepted_ = false;
    states_.push(0);
    history_ = "";
}

APTnode to_APTnode(const Token &token) {
    APTnode node;
    node.id = token.id;
    node.attr.Set<string>("lval", token.lval);
    return node;
}

void SyntaxParser::parseStep(Token token) {
    inputs_.push_back(to_APTnode(token));
    parse();
}

void SyntaxParser::parseStep(APTnode tokenWithAttr) {
    inputs_.push_back(tokenWithAttr);
    parse();
}

void SyntaxParser::parseAll(vector<Token> tokens) {
    vector<APTnode> tokensWithAttr(tokens.size());
    for (int i = 0; i < tokens.size(); i++) {
        tokensWithAttr[i] = to_APTnode(tokens[i]);
    }
    inputs_.insert(inputs_.end(), tokensWithAttr.begin(), tokensWithAttr.end());
    parse();
}

void SyntaxParser::parseAll(vector<APTnode> tokensWithAttr) {
    inputs_.insert(inputs_.end(), tokensWithAttr.begin(), tokensWithAttr.end());
    parse();
}

shared_ptr<APTnode> SyntaxParser::getAPT() {
    if (!isAccepted_) { return nullptr; }
    assert(nodes_.size() == 1);

    shared_ptr<APTnode> root = nodes_.top();
    return root;
}

void printAPT_(const APTnode *const node, ostream &oss,
               const map<int, string> &symbolNames, bool skipMid, bool skipAttr,
               vector<bool> isLast = {}) {
    // if (node == nullptr) { return; }
    if (skipMid && node->child.size() == 1) {
        printAPT_(node->child[0].get(), oss, symbolNames, skipMid, skipAttr,
                  isLast);
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
    auto & attr = node->attr;
    string attr_str =
        skipAttr
            ? (attr.Has<string>("lval") ? attr.Get<string>("lval") : string())
            : attr.str2();
    oss << fmt::format("{:16s}  {}\n",
                       fmt::format("<{:s}>", symbolNames.at(node->id)),
                       attr_str);

    isLast.push_back(false);
    for (auto it = node->child.begin(); it != node->child.end(); it++) {
        if (it + 1 == node->child.end()) { isLast.back() = true; }
        printAPT_((*it).get(), oss, symbolNames, skipMid, skipAttr, isLast);
    }
}

string getAPTstr(const shared_ptr<APTnode> &root, const Grammar &grammar) {
    bool         skipMid  = false;
    bool         skipAttr = false;
    stringstream ss;
    printAPT_(root.get(), ss, grammar.symbolNames, skipMid, skipAttr);
    return ss.str();
}

string getASTstr(const shared_ptr<APTnode> &root, const Grammar &grammar) {
    bool         skipMid  = true;
    bool         skipAttr = true;
    stringstream ss;
    printAPT_(root.get(), ss, grammar.symbolNames, skipMid, skipAttr);
    return ss.str();
}

string SyntaxParser::getAPTstr() {
    vector<shared_ptr<APTnode>> unrootedNodes = to_vector(nodes_);
    stringstream                ss;
    for (const auto &node : unrootedNodes) {
        ss << runtime::getAPTstr(node, grammar_);
    }
    return ss.str();
}

string SyntaxParser::getASTstr() {
    vector<shared_ptr<APTnode>> unrootedNodes = to_vector(nodes_);
    stringstream                ss;
    for (const auto &node : unrootedNodes) {
        ss << runtime::getASTstr(node, grammar_);
    }
    return ss.str();
}

string SyntaxParser::getErrorMessage() {
    auto & tokenWithAttr = inputs_.at(offset_);
    string errorMsg      = fmt::format(
        "Syntax Parsing Error: unexpected token <{} \"{}\">",
        grammar_.symbolNames.at(tokenWithAttr.id),
        fmt::format(fmt::emphasis::underline, "{}",
                    unescape(tokenWithAttr.attr.Get<string>("lval"))));

    logger.error("{}", errorMsg);
    logger.error("  history_: \"{}\"", history_);
    logger.error("  look: {} \"{}\"", grammar_.symbolNames.at(tokenWithAttr.id),
                 unescape(tokenWithAttr.attr.Get<string>("lval")));
    logger.error(
        "  symbols: [{}]",
        fmt::join(apply_map(to_vector(symbols_), grammar_.symbolNames), ","));
    logger.error("  states: [{}]", fmt::join(to_vector(states_), ","));
    logger.error("Current AST: \n{}", getASTstr());

    return errorMsg;
}

} // namespace krill::runtime