#include "krill/grammar.h"
#include "krill/defs.h"
#include <cassert>
#include <fmt/format.h>
#include <iostream>
#include <vector>
using namespace std;
using namespace krill::type;
using namespace krill::grammar;


void printProd(const Prod &prod, map<int, string> symbolNames, ostream &oss) {
    oss << symbolNames[prod.symbol];
    oss << " -> ";
    for (int i = 0; i < prod.right.size(); i++) {
        oss << symbolNames[prod.right[i]] << " ";
    }
    oss << "\n";
}

void printGrammar(const Grammar &grammar, ostream &oss) {
    oss << "Grammar: \n";
    for (int i = 0; i < grammar.prods.size(); i++) {
        oss << fmt::format("({:d}) ", i + 1);
        printProd(grammar.prods[i], grammar.symbolNames, oss);
    }
}

void printFirstSet(const map<int, set<int>> &firstSet, ostream &oss) {
    for (auto[symbol, nextSymbols] : firstSet) {
        oss << symbol;
        oss << ": {";
        if (nextSymbols.size() != 0) {
            auto it = nextSymbols.begin();
            oss << *it;
            for (it++; it != nextSymbols.end(); it++) {
                oss << ", ";
                oss << *it;
            }
        }
        oss << "} \n";
    }
}

void printActionTable(const ActionTable &actionTable,
                      map<int, string> symbolNames, ostream &oss) {
    oss << fmt::format("Analysis Table (size={}): \n", actionTable.size());
    string typeName[] = {"ACTION", "REDUCE", "GOTO  ", "ACCEPT"};
    for (auto[key, action] : actionTable) {
        oss << fmt::format("s{:<2d} --> {:s} --> {:<6s} ", key.first,
                           key.second != END_SYMBOL ? symbolNames[key.second]
                                                    : "[end]",
                           typeName[action.type]);
        if (action.type == ACTION || action.type == GOTO) {
            oss << fmt::format("s{:<2d}", action.tgt);
        } else if (action.type == REDUCE) {
            oss << fmt::format("r{:<2d}", action.tgt);
        }
        oss << "\n";
    }
}

void printLR1State(LR1State state, map<int, string> symbolNames, ostream &oss) {
    for (ProdLR1Item item : state) {
        oss << fmt::format("  {} ->", symbolNames.at(item.symbol));
        for (int i = 0; i < item.dot; i++) {
            oss << fmt::format(" {}", symbolNames.at(item.right[i]));
        }
        oss << " .";
        for (int i = item.dot; i < item.right.size(); i++) {
            oss << fmt::format(" {}", symbolNames.at(item.right[i]));
        }
        oss << fmt::format(" | {} ({})", symbolNames[item.search], item.search);
        oss << "\n";
    }
}

void printEdgeTable(EdgeTable edgeTable, map<int, string> symbolNames, ostream &oss) {
    oss << "EdgeTable: \n";
    for (const Edge &edge : edgeTable) {
        oss << fmt::format("s{:<2d} --> {:s} --> s{:<2d}\n", 
                           edge.from, symbolNames.at(edge.symbol), edge.to);
    }
}


void printLr1Automata(LR1Automata lr1Automata, map<int, string> symbolNames, ostream &oss) {
    int i = 0;
    for (LR1State state : lr1Automata.states) {
        cout << fmt::format("{}): \n", i++);
        printLR1State(state, symbolNames, oss);
        cout << "\n";
    }
    printEdgeTable(lr1Automata.edgeTable, symbolNames, oss);
}

// test first set and follow set
void test1() {
    printf("test first-set and follow-set \n");
    printf("----------------------------- \n");
    // initialize symbolset and grammar
    Grammar grammar({
        "Q -> S",
        "S -> V '=' R",
        "S -> R",
        "R -> L",
        "L -> '*' R",
        "L -> i",
        "V -> a",
    });
    printf("> initial grammar: \n");
    printGrammar(grammar, cout);

    auto firstSet  = getFirstSet(grammar);
    auto followSet = getFollowSet(grammar, firstSet);

    printf("> first-set: \n");
    printFirstSet(firstSet, cout);

    printf("> follow-set: \n");
    printFirstSet(followSet, cout);

    printf("\n");
}

// 语法树节点
struct GrammarNode {
    vector<GrammarNode *> children;
    int                   symbol;
};

vector<int> simpleLexicalParser(map<int, string> symbolNames, string src) {
    map<char, int> symbolNamesRev;
    for (auto[id, str] : symbolNames) { symbolNamesRev[str[0]] = id; }

    vector<int> tokens;
    for (char c : src) {
        assert(symbolNamesRev.count(c) > 0);
        tokens.push_back(symbolNamesRev[c]);
    }
    return tokens;
}

GrammarNode *simpleSyntaxParser(vector<Prod> prods, ActionTable actionTable,
                                vector<int> src, bool showStates = false) {
    if (src[src.size() - 1] != END_SYMBOL) { src.push_back(END_SYMBOL); }

    // stack of states
    vector<int>           states;
    vector<GrammarNode *> stateNodes;
    states.push_back(0);

    for (int i = 0, flag = true; flag;) {
        // look src[i], state => next_state, action
        if (showStates) {
            cout << fmt::format("states: [{}], look: {}\n",
                                fmt::join(states, ", "), src[i]);
        }

        assert(actionTable.count({states.back(), src[i]}) != 0);

        Action action = actionTable[{states.back(), src[i]}];
        switch (action.type) {
            case ACTION: {
                states.push_back(action.tgt);
                stateNodes.push_back(new GrammarNode({{}, src[i]}));
                i++;
                break;
            }
            case REDUCE: {
                Prod                  r = prods[action.tgt];
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
            case ACCEPT: {
                flag = false;
                break;
            }
            default: {
                assert(false);
                break;
            }
        }
        if (showStates) {
            cout << fmt::format("states: [{}], done\n",
                                fmt::join(states, ", "));
        }
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
    printf("test LR(1) analysis procedure \n");
    printf("----------------------------- \n");
    // initialize symbolset and grammar
    Grammar grammar({
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
    printGrammar(grammar, cout);
    printf("> symbol names:\n");
    for (auto[id, str] : grammar.symbolNames) {
        cout << fmt::format("{:d}: {:s}\n", id, str);
    }

    auto firstSet  = getFirstSet(grammar);
    auto followSet = getFollowSet(grammar, firstSet);

    printf("> first-set: \n");
    printFirstSet(firstSet, cout);
    printf("> follow-set: \n");
    printFirstSet(followSet, cout);

    auto lr1Automata = getLR1Automata(grammar);
    auto actionTable = getLR1table(grammar, lr1Automata);
    printf("> Action Table of LR(1) dfa: \n");
    printActionTable(actionTable, grammar.symbolNames, cout);

    printf("> use LR(1) Action Table to analyze: \n");
    string src = "(d+d*d)*d+(d)";
    fmt::print("> input string: {}\n", src);
    vector<int> tokens = simpleLexicalParser(grammar.symbolNames, src);
    printf("%s\n", src.c_str());
    for (int i : tokens) { printf("%d ", i); }
    printf("\n");

    GrammarNode *root =
        simpleSyntaxParser(grammar.prods, actionTable, tokens, true);
    printf("> grammar tree: \n");
    printGrammarTreeNode(root, grammar.symbolNames, cout);

    printf("\n");
}

// test syntax parsing
void test3() {
    printf("test LR(1) analysis procedure \n");
    printf("----------------------------- \n");
    // initialize symbolset and grammar
    // Grammar grammar({
    //     "P -> ( As Bs )", 
    //     "As -> As A", // 左递归
    //     "As ->", 
    //     "Bs -> Bs B", // 左递归
    //     "Bs ->", 
    // };
    // Grammar grammar({
    //     // 只要有空产生式就容易挂
    //     "P -> ( As Bs )", 
    //     "As -> A As",
    //     "As ->", 
    //     "Bs -> B Bs",
    //     "Bs ->", 
    // });
    Grammar grammar({
        "S -> P", 
        "P -> ( As Bs )", 
        "P -> ( As )", 
        "P -> ( Bs )", 
        "P -> ( )", 
        "As -> A As",
        "As -> A", 
        "Bs -> B Bs",
        "Bs -> B", 
    });
    printGrammar(grammar, cout);
    printf("> symbol names:\n");
    for (auto[id, str] : grammar.symbolNames) {
        cout << fmt::format("{:d}: {:s}\n", id, str);
    }

    auto firstSet  = getFirstSet(grammar);
    auto followSet = getFollowSet(grammar, firstSet);

    printf("> first-set: \n");
    printFirstSet(firstSet, cout);
    printf("> follow-set: \n");
    printFirstSet(followSet, cout);

    auto lr1Automata = getLR1Automata(grammar);
    printf("> LR1 Automata: \n");
    printLr1Automata(lr1Automata, grammar.symbolNames, cout);

    auto actionTable = getLR1table(grammar, lr1Automata);
    printf("> Action Table of LR(1) dfa: \n");
    printActionTable(actionTable, grammar.symbolNames, cout);



    printf("> use LR(1) Action Table to analyze: \n");
    string src = "()";
    fmt::print("> input string: {}\n", src);
    vector<int> tokens = simpleLexicalParser(grammar.symbolNames, src);
    printf("%s\n", src.c_str());
    for (int i : tokens) { printf("%d ", i); }
    printf("\n");

    GrammarNode *root =
        simpleSyntaxParser(grammar.prods, actionTable, tokens, true);
    printf("> grammar tree: \n");
    printGrammarTreeNode(root, grammar.symbolNames, cout);

    printf("\n");
}

int main() {
    // vector<void (*)()> testFuncs = {test1, test2, test3};
    vector<void (*)()> testFuncs = { test3};
    for (int i = 0; i < testFuncs.size(); i++) {
        cout << "#test " << (i + 1) << endl;
        testFuncs[i]();
        cout << endl << endl;
    }
    return 0;
}
