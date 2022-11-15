#include "krill/syntax.h"
#include "krill/grammar.h"
#include "fmt/format.h"
#include <cassert>
#include <map>
#include <iostream>
#include <string>
#include <vector>
using namespace krill::grammar;
using namespace std;

namespace krill::runtime {

void defaultReduceFunc(AttrDict &next, deque<AttrDict> &child) {
    // pass
}

void defaultActionFunc(AttrDict &next, const Token &token) {
    next.Set<string>("lval", token.lval);
}

SyntaxParser::SyntaxParser(Grammar grammar, ActionTable actionTable)
    : grammar_(grammar), actionTable_(actionTable), offset_(0),
      isAccepted_(false), states_({0}), history_("") {
    actionFunc_ = defaultActionFunc;
    for (int i = 0; i < grammar.prods.size(); i++) {
        reduceFunc_.push_back(defaultReduceFunc);
    }
}

void SyntaxParser::parse() {
    auto &prods_ = grammar_.prods;

    while (!isAccepted_ && offset_ < tokens_.size()) {
        Token &token = tokens_.at(offset_);

        assert(states_.size() > 0);
        if (actionTable_.count({states_.top(), token.id}) == 0) {
            throw runtime_error(fmt::format(
                "Syntax Error: unexpected token \"{}\" after \"{}\"\n",
                token.lval, history_));
        }

        assert(actionTable_.count({states_.top(), token.id}) != 0);
        Action action = actionTable_[{states_.top(), token.id}];

        switch (action.type) {
            case ACTION: {
                states_.push(action.tgt);

                shared_ptr<APTnode> nextNode(
                    new APTnode({.id = token.id, .attr = {}, .child = {}}));
                // ACTION action
                actionFunc_(nextNode.get()->attr, token);
                nodes_.push(nextNode);

                history_ += token.lval;
                if (history_.size() > 20) {
                    history_ = history_.substr(10);
                }

                offset_++;
                break;
            }
            case REDUCE: {
                Prod              r = prods_[action.tgt];
                deque<shared_ptr<APTnode>> childNodes;
                deque<AttrDict> childAttrs;

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
    tokens_.clear();
    states_ = stack<int>();
    nodes_ = stack<shared_ptr<APTnode>>();
    offset_  = 0;
    isAccepted_ = false;
    states_.push(0);
    history_ = "";
}

void SyntaxParser::parseStep(Token token) {
    tokens_.push_back(token);
    parse();
}

void SyntaxParser::parseAll(vector<Token> tokens) {
    tokens_.insert(tokens_.end(), tokens.begin(), tokens.end());
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
    if (node == nullptr) { return; }
    if (forShort) {
        if (node->child.size() == 1) {
            printAPT_(node->child[0].get(), oss, symbolNames, forShort, isLast);
            return;
        }
    }
    if (isLast.size() == 0) { oss << fmt::format(" "); }

    for (int i = 0; i < isLast.size(); i++) {
        if (i + 1 == isLast.size()) {
            oss << fmt::format("{:s}", (isLast[i] ? " └─ " : " ├─ "));
        } else {
            oss << fmt::format("{:s}", (isLast[i] ? "   " : " │ "));
        }
    }
    if (node->child.size() == 0) {
        oss << fmt::format("{:s} \"{:s}\"\n", symbolNames.at(node->id),
                           node->attr.Get<string>("lval"));
    } else {
        oss << fmt::format("{:s}\n", symbolNames.at(node->id));
    }
    isLast.push_back(false);
    for (auto it = node->child.begin(); it != node->child.end(); it++) {
        if (it + 1 == node->child.end()) { isLast.back() = true; }
        printAPT_((*it).get(), oss, symbolNames, forShort, isLast);
    }
}

void SyntaxParser::printAnnotatedParsingTree(ostream &oss) {
    if (!isAccepted_) { return; }
    APTnode *root     = getAnnotatedParsingTree();
    bool     forShort = true;
    printAPT_(root, oss, grammar_.symbolNames, forShort);
}

} // namespace krill::runtime