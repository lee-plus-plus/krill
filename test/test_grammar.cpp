#include "krill/defs.h"
#include "Krill/grammar.h"
#include "krill/utils.h"
#include <fmt/format.h>
#include <cassert>
#include <iostream>
#include <vector>
using namespace std;
using namespace krill::grammar;
using namespace krill::grammar::core;
using namespace krill::utils;

// test first set and follow set
void test1() {
    printf("test first-set and follow-set \n");
    printf("----------------------------- \n");
    // initialize symbolset and grammar
    auto[grammar, symbolNames] = getGrammarFromStr({
        "Q -> S",
        "S -> V = R",
        "S -> R",
        "R -> L",
        "L -> * R",
        "L -> i",
        "V -> a",
    });
    printf("> initial grammar: \n");
    printGrammar(grammar, symbolNames, cout);

    auto firstSet  = getFirstSet(grammar);
    auto followSet = getFollowSet(grammar, firstSet);

    printf("> first-set: \n");
    printFirstset(firstSet, cout);

    printf("> follow-set: \n");
    printFirstset(followSet, cout);

    printf("\n");
}

// 语法树节点
struct GrammarNode {
    vector<GrammarNode *> children;
    int symbol;
};

vector<int> simpleLexicalParser(map<int, string> symbolNames, string src) {
    map<char, int> symbolNamesRev;
    for (auto [id, str] : symbolNames) {
        symbolNamesRev[str[0]] = id;
    }

    vector<int> tokens;
    for (char c : src) {
        assert(symbolNamesRev.count(c) > 0);
        tokens.push_back(symbolNamesRev[c]);
    }
    return tokens;
}

GrammarNode *simpleSyntaxParser(vector<Prod> prods, ActionTable actionTable,
                               vector<int> src) {
    if (src[src.size() - 1] != END_SYMBOL) { src.push_back(END_SYMBOL); }

    // stack of states
    vector<int> states;
    vector<GrammarNode *> stateNodes;
    states.push_back(0);

    for (int i = 0, flag = true; flag;) {
        // look src[i], state => next_state, action
        cout << fmt::format("states: [{}], look: {}\n", fmt::join(states, ", "), src[i]);

        assert(actionTable.count({states.back(), src[i]}) != 0);

        Action action = actionTable[{states.back(), src[i]}];
        switch (action.type) {
            case Action::ACTION: {
                states.push_back(action.tgt);
                stateNodes.push_back(new GrammarNode({{}, src[i]}));
                i++;
                break;
            }
            case Action::REDUCE: {
                Prod r = prods[action.tgt];
                vector<GrammarNode *> temp;
                for (int j = 0; j < r.right.size(); j++) {
                    temp.push_back(stateNodes.back());
                    states.pop_back();
                    stateNodes.pop_back();
                }

                assert(actionTable.count({states.back(), r.symbol}) != 0);
                Action action2 = actionTable[{states.back(), r.symbol}];
                states.push_back(action2.tgt);

                GrammarNode *nextNode = new GrammarNode({{}, r.symbol});
                while (temp.size()) {
                    nextNode->children.push_back(temp.back());
                    temp.pop_back();
                }
                stateNodes.push_back(nextNode);
                break;
            }
            case Action::ACCEPT: {
                flag = false;
                break;
            }
            default: {
                assert(false);
                break;
            }
        }

        cout << fmt::format("states: [{}], done\n", fmt::join(states, ", "));
    }
    assert(stateNodes.size() == 1);
    return stateNodes.back();
}

void printGrammarTreeNode(GrammarNode *node, map<int, string> symbolNames,
                          ostream &oss, vector<bool> isLast = {}) {
    if (node->children.size() == 1) {
        printGrammarTreeNode(node->children[0], symbolNames, oss, isLast);
        return;
    }
    if (isLast.size() == 0) { printf(" "); }

    for (int i = 0; i < isLast.size(); i++) {
        if (i + 1 == isLast.size()) {
            printf("%s", (isLast[i] ? " └─ " : " ├─ "));
        } else {
            printf("%s", (isLast[i] ? "   " : " │ "));
        }
    }
    if (node->children.size() == 0) {
        printf("%s\n", symbolNames[node->symbol].c_str());
    } else {
        printf("%s\n", symbolNames[node->symbol].c_str());
    }
    isLast.push_back(false);
    for (auto it = node->children.begin(); it != node->children.end(); it++) {
        if (it + 1 == node->children.end()) { isLast.back() = true; }
        printGrammarTreeNode(*it, symbolNames, oss, isLast);
    }
}

// test syntax parsing
void test2() {
    printf("test LR(1) analysis \n");
    printf("------------------- \n");
    // initialize symbolset and grammar
    auto [grammar, symbolNames] = getGrammarFromStr({
        "Q -> P",
        "P -> T",
        "T -> ( P )",
        "T -> T * T",
        "T -> T / T",
        "P -> T + T",
        "P -> T - T",
        "T -> - T",
        "T -> d",
    });
    printGrammar(grammar, symbolNames, cout);
    printf("> symbol names:\n");
    for (auto [id, str] : symbolNames) {
        cout << fmt::format("{:d}: {:s}\n", id, str);
    }

    auto firstSet  = getFirstSet(grammar);
    auto followSet = getFollowSet(grammar, firstSet);

    printf("> first-set: \n");
    printFirstset(firstSet, cout);
    printf("> follow-set: \n");
    printFirstset(followSet, cout);

    auto [covers, edgeTable] = getLR1dfa(grammar);
    // auto [covers, edgeTable]   = getLALR1fromLR1(grammar, covers0, edgeTable0);
    // printf("> covers of LR(1): \n");
    // for (int i = 0; i < covers.size(); i++) {
    //     printf("c%d: \n", i);
    //     for (ProdLR1Item prod : covers[i]) { printProdLR1Item(prod); }
    // }

    // print edge table
    // printf("> edge table of LR(1) dfa: \n");
    // printEdgeTable(edgeTable);

    auto actionTable = getLR1table(grammar, covers, edgeTable);
    printf("> analysis table of LR(1) dfa: \n");
    printActionTable(actionTable, symbolNames, cout);

    printf("> use LR(1) analysis table to analyze: \n");
    printf("> input string: \n");
    string src = "(d+d*d)*d+(d)";
    vector<int> tokens = simpleLexicalParser(symbolNames, src);
    printf("%s\n", src.c_str());
    for (int i : tokens) {
        printf("%d ", i);
    }
    printf("\n");


    GrammarNode *root =
        simpleSyntaxParser(grammar.prods, actionTable, tokens);
    printf("> grammar tree: \n");
    printGrammarTreeNode(root, symbolNames, cout);

    printf("\n");
}

int main() {
    vector<void (*)()> testFuncs = {test1, test2};
    for (int i = 0; i < testFuncs.size(); i++) {
        cout << "#test " << i << endl;
        testFuncs[i]();
        cout << endl << endl;
    }
    return 0;
}
