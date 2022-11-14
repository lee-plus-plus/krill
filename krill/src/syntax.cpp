#include "krill/syntax.h"
#include "fmt/format.h"
#include "krill/automata.h"
#include "krill/grammar.h"
#include <cassert>
#include <map>
#include <ostream>
#include <string>
#include <vector>
using namespace krill::grammar;
using namespace std;

namespace krill::runtime {

SyntaxParser::SyntaxParser(Grammar grammar, ActionTable actionTable)
    : grammar_(grammar), actionTable_(actionTable), posTokens_(0),
      isAccepted_(false), states_({0}) {}

void SyntaxParser::parse() {
    auto &prods_ = grammar_.prods;

    while (!isAccepted_ && posTokens_ < tokens_.size()) {
        Token &token = tokens_.at(posTokens_);

        assert(states_.size() > 0);
        assert(actionTable_.count({states_.back(), token.id}) != 0);
        Action action = actionTable_[{states_.back(), token.id}];

        switch (action.type) {
            case Action::ACTION: {
                states_.push_back(action.tgt);
                nodes_.push_back(new APTnode({.syntaxId = token.id,
                                              .lexValue = token.lexValue,
                                              .child    = {}}));
                posTokens_++;
                break;
            }
            case Action::REDUCE: {
                Prod              r = prods_[action.tgt];
                vector<APTnode *> childNodes;
                for (int j = 0; (int) j < r.right.size(); j++) {
                    states_.pop_back();
                    childNodes.insert(childNodes.begin(), nodes_.back());
                    nodes_.pop_back();
                }

                assert(actionTable_.count({states_.back(), r.symbol}) != 0);
                Action action2 = actionTable_[{states_.back(), r.symbol}];
                assert(action2.type == Action::GOTO);
                states_.push_back(action2.tgt);

                assert(0 <= action.tgt && action.tgt < prods_.size());
                APTnode *nextNode =
                    new APTnode({.syntaxId = prods_.at(action.tgt).symbol,
                                 .lexValue = "",
                                 .child    = childNodes});

                nodes_.push_back(nextNode);
                break;
            }
            case Action::ACCEPT: {
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

void SyntaxParser::reset() {
    tokens_.clear();
    states_.clear();
    nodes_.clear();
    posTokens_  = 0;
    isAccepted_ = false;
    states_.push_back(0);
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

    APTnode *root = nodes_.front();
    return root;
}

void printAPT_(APTnode *node, ostream &oss, map<int, string> symbolNames,
               bool forShort = false, vector<bool> isLast = {}) {
    if (node == nullptr) { return; }
    if (node->child.size() == 1) {
        printAPT_(node->child[0], oss, symbolNames, forShort, isLast);
        return;
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
        oss << fmt::format("{:s} \"{:s}\"\n", symbolNames.at(node->syntaxId),
                           node->lexValue);
    } else {
        oss << fmt::format("{:s}\n", symbolNames.at(node->syntaxId));
    }
    isLast.push_back(false);
    for (auto it = node->child.begin(); it != node->child.end(); it++) {
        if (it + 1 == node->child.end()) { isLast.back() = true; }
        printAPT_(*it, oss, symbolNames, forShort, isLast);
    }
}

void SyntaxParser::printAnnotatedParsingTree(ostream &oss) {
    if (!isAccepted_) { return; }
    APTnode *root     = getAnnotatedParsingTree();
    bool     forShort = false;
    printAPT_(root, oss, grammar_.symbolNames, forShort);
}

} // namespace krill::runtime