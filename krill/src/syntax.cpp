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
using namespace krill::type;
using namespace krill::grammar;
using namespace krill::utils;
using namespace std;

namespace krill::runtime {

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
    auto &prods_ = grammar_.prods;

    while (!isAccepted_ && offset_ < inputs_.size()) {
        APTnode &input = inputs_.at(offset_);
        assert(input.attr.Has<string>("lval"));

        assert(states_.size() > 0);
        if (actionTable_.count({states_.top(), input.id}) == 0) {
            throw runtime_error(getErrorMessage());
        }

        assert(actionTable_.count({states_.top(), input.id}) != 0);
        Action action = actionTable_[{states_.top(), input.id}];

        switch (action.type) {
            case ACTION: {
                states_.push(action.tgt);

                shared_ptr<APTnode> nextNode(new APTnode(input));
                // ACTION action
                actionFunc_(nextNode.get()->attr, input.attr);
                nodes_.push(nextNode);

                history_ += input.attr.Get<string>("lval");
                if (history_.size() > 30) {
                    history_ = history_.substr(history_.size() - 30);
                }

                offset_++;
                break;
            }
            case REDUCE: {
                Prod                       r = prods_[action.tgt];
                deque<shared_ptr<APTnode>> childNodes;
                deque<AttrDict>            childAttrs;

                for (int j = 0; (int) j < r.right.size(); j++) {
                    states_.pop();
                    childNodes.push_front(nodes_.top());
                    childAttrs.push_front(nodes_.top()->attr);
                    nodes_.pop();
                }

                assert(actionTable_.count({states_.top(), r.symbol}) != 0);
                Action action2 = actionTable_[{states_.top(), r.symbol}];
                assert(action2.type == GOTO);
                states_.push(action2.tgt);
                assert(0 <= action.tgt && action.tgt < prods_.size());

                shared_ptr<APTnode> nextNode(
                    new APTnode({.id    = prods_.at(action.tgt).symbol,
                                 .attr  = {},
                                 .child = childNodes}));
                // REDUCE action
                reduceFunc_[action.tgt](nextNode.get()->attr, childAttrs);
                for (int j = 0; (int) j < r.right.size(); j++) {
                    childNodes[j].get()->attr = childAttrs[j];
                }

                nodes_.push(nextNode);
                break;
            }
            case ACCEPT: {
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

APTnode *SyntaxParser::getAnnotatedParsingTree() {
    if (!isAccepted_) { return nullptr; }
    assert(nodes_.size() == 1);

    APTnode *root = nodes_.top().get();
    return root;
}

void printAPT_(APTnode *node, ostream &oss, map<int, string> symbolNames,
               bool forShort, vector<bool> isLast = {}) {
    // if (node == nullptr) { return; }
    if (forShort && node->child.size() == 1) {
        printAPT_(node->child[0].get(), oss, symbolNames, forShort, isLast);
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
        string attr_str = attr.str();
        // string attr_str = fmt::format(
        //     fmt::emphasis::underline, "\"{}\"",
        //     attr.Has<string>("lval") ? unescape(attr.Get<string>("lval")) :
        //     "");
        oss << fmt::format("<{:s}> {}\n", symbolNames.at(node->id), attr_str);
    } else {
        oss << fmt::format("<{:s}>\n", symbolNames.at(node->id));
    }
    isLast.push_back(false);
    for (auto it = node->child.begin(); it != node->child.end(); it++) {
        if (it + 1 == node->child.end()) { isLast.back() = true; }
        printAPT_((*it).get(), oss, symbolNames, forShort, isLast);
    }
}

void SyntaxParser::printAnnotatedParsingTree(ostream &oss) {
    // if (!isAccepted_) { return; }
    // APTnode *root     = getAnnotatedParsingTree();
    bool                        forShort      = false;
    vector<shared_ptr<APTnode>> unrootedNodes = to_vector(nodes_);
    for (auto node : unrootedNodes) {
        printAPT_(node.get(), oss, grammar_.symbolNames, forShort);
    }
}

string SyntaxParser::getErrorMessage() {
    auto &tokenWithAttr = inputs_.at(offset_);

    stringstream ss;
    ss << fmt::format("{}: unexpected token <{} \"{}\"> after \"{}\"",
                      fmt::format("Syntax Error", fmt::color::red),
                      grammar_.symbolNames.at(tokenWithAttr.id),
                      fmt::format(fmt::emphasis::underline, "{}",
                                  krill::utils::unescape(
                                      tokenWithAttr.attr.Get<string>("lval"))),
                      fmt::format(fmt::emphasis::underline, "{}",
                                  krill::utils::unescape(history_)));
    ss << "\n";
    printAnnotatedParsingTree(ss);
    return ss.str();
}

} // namespace krill::runtime