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

void defaultReduceFunc(AttrDict &next, deque<AttrDict> &child) {
    // pass
}

void defaultActionFunc(AttrDict &next, AttrDict &child) { next = child; }

SyntaxParser::SyntaxParser(Grammar grammar, ActionTable actionTable)
    : grammar_(grammar), actionTable_(actionTable), states_({0}), offset_(0),
      isAccepted_(false), history_("") {
    actionFunc_ = defaultActionFunc;
    for (int i = 0; i < grammar.prods.size(); i++) {
        reduceFunc_.push_back(defaultReduceFunc);
    }
}

SyntaxParser::SyntaxParser(Grammar grammar, ActionTable actionTable,
                           Afunc actionFunc, vector<Rfunc> reduceFunc)
    : actionFunc_(actionFunc), reduceFunc_(reduceFunc), grammar_(grammar),
      actionTable_(actionTable), states_({0}), offset_(0), isAccepted_(false),
      history_("") {
    assert(reduceFunc_.size() == grammar.prods.size());
}

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
                // ACTION action
                actionFunc_(nextNode.get()->attr, input.attr);
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
                deque<AttrDict>            childAttrs;

                auto poped_states = get_top(states_, r.right.size());
                for (int j = 0; (int) j < r.right.size(); j++) {
                    states_.pop();
                    symbols_.pop();
                    childNodes.push_front(nodes_.top());
                    childAttrs.push_front(nodes_.top()->attr);
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

                shared_ptr<APTnode> nextNode(new APTnode(
                    {.id = r.symbol, .attr = {}, .child = childNodes}));
                // REDUCE action
                reduceFunc_[action.tgt](nextNode.get()->attr, childAttrs);
                for (int j = 0; (int) j < r.right.size(); j++) {
                    childNodes[j].get()->attr = childAttrs[j];
                }

                nodes_.push(nextNode);
                break;
            }
            case Action::Type::kAccept: {
                logger.debug("  ACCEPT");
                logger.info("syntax parsing complete successfully");
                stringstream ss_ast;
                printAPT(ss_ast);
                logger.debug("Current AST: \n{}", ss_ast.str());
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

APTnode *SyntaxParser::getAPT() {
    if (!isAccepted_) { return nullptr; }
    assert(nodes_.size() == 1);

    APTnode *root = nodes_.top().get();
    return root;
}

void printAPT_(APTnode *node, ostream &oss, map<int, string> symbolNames,
               bool skipMid, bool skipAttr, vector<bool> isLast = {}) {
    // if (node == nullptr) { return; }
    if (skipMid && node->child.size() == 1) {
        printAPT_(node->child[0].get(), oss, symbolNames, skipMid, skipAttr, isLast);
        return;
    }
    if (isLast.size() == 0) { oss << fmt::format(" "); }

    for (int i = 0; i < isLast.size(); i++) {
        if (i + 1 == isLast.size()) {
            oss << fmt::format("{:s}", (isLast[i] ? " └ " : " ├ "));
        } else {
            oss << fmt::format("{:s}", (isLast[i] ? "   " : " │ "));
        }
    }
    if (node->child.size() == 0) {
        auto & attr     = node->attr;
        string attr_str = skipAttr ? (attr.Has<string>("lval") ? attr.Get<string>("lval") : string()) : attr.str();
        // string attr_str = fmt::format(
        //     fmt::emphasis::underline, "\"{}\"",
        //     attr.Has<string>("lval") ? unescape(attr.Get<string>("lval")) :
        //     "");
        oss << fmt::format("<{:s}>  {}\n", symbolNames.at(node->id), attr_str);
    } else {
        oss << fmt::format("<{:s}>\n", symbolNames.at(node->id));
    }
    isLast.push_back(false);
    for (auto it = node->child.begin(); it != node->child.end(); it++) {
        if (it + 1 == node->child.end()) { isLast.back() = true; }
        printAPT_((*it).get(), oss, symbolNames, skipMid, skipAttr, isLast);
    }
}

void SyntaxParser::printAPT(ostream &oss) {
    bool skipMid = false;
    bool skipAttr = false;
    vector<shared_ptr<APTnode>> unrootedNodes = to_vector(nodes_);
    for (auto node : unrootedNodes) {
        printAPT_(node.get(), oss, grammar_.symbolNames, skipMid, skipAttr);
    }
}

void SyntaxParser::printAST(ostream &oss) {
    bool skipMid = true;
    bool skipAttr = true;
    vector<shared_ptr<APTnode>> unrootedNodes = to_vector(nodes_);
    for (auto node : unrootedNodes) {
        printAPT_(node.get(), oss, grammar_.symbolNames, skipMid, skipAttr);
    }
}

string SyntaxParser::getAPTstr() {
    stringstream ss;
    printAPT(ss);
    return ss.str();
}

string SyntaxParser::getASTstr() {
    stringstream ss;
    printAST(ss);
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
    logger.error("  look: {} \"{}\"", grammar_.symbolNames.at(tokenWithAttr.id), unescape(tokenWithAttr.attr.Get<string>("lval")));
    logger.error(
        "  symbols: [{}]",
        fmt::join(apply_map(to_vector(symbols_), grammar_.symbolNames), ","));
    logger.error("  states: [{}]", fmt::join(to_vector(states_), ","));

    stringstream ss_ast;
    printAPT(ss_ast);
    logger.error("Current AST: \n{}", ss_ast.str());

    return errorMsg;
}

} // namespace krill::runtime