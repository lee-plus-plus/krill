#include "krill/defs.h"
#include "krill/grammar.h"
#include "spdlog/spdlog.h"
#include <cassert>
#include <fmt/format.h>
#include <iostream>
#include <vector>
using namespace std;
using namespace krill::type;
using namespace krill::grammar;

struct GrammarNode {
    vector<GrammarNode *> children;
    int                   symbol;
};

// accept single char as symbol
vector<int> simpleLexicalParser(const map<int, string> &symbolNames,
                                string                  src) {
    map<char, int> symbolNamesRev;
    for (auto[id, str] : symbolNames) { symbolNamesRev[str[0]] = id; }

    vector<int> tokens;
    for (char c : src) {
        assert(symbolNamesRev.count(c) > 0);
        tokens.push_back(symbolNamesRev[c]);
    }
    return tokens;
}

GrammarNode *simpleSyntaxParser(Grammar grammar, ActionTable actionTable,
                                vector<int> src, bool showStates = false) {
    if (src[src.size() - 1] != END_SYMBOL) { src.push_back(END_SYMBOL); }

    const auto &prods       = grammar.prods;
    auto        symbolNames = grammar.symbolNames;

    // stack of states
    vector<int>           states;
    vector<GrammarNode *> stateNodes;
    states.push_back(0);

    auto getStateNodesNames =
        [symbolNames](vector<GrammarNode *> stateNodes) -> vector<string> {
        vector<string> names;
        for (auto it : stateNodes) {
            names.push_back(symbolNames.at(it->symbol));
        }
        return names;
    };

    for (int i = 0, flag = true; flag;) {
        // look src[i], state => next_state, action
        if (showStates) {
            cout << fmt::format("states: [{}], look: {}\t\t",
                                fmt::join(states, ", "),
                                symbolNames.at(src[i]));
        }

        assert(actionTable.count({states.back(), src[i]}) != 0);
        if (states.size() >= 50) { throw runtime_error("syntax parsing dead"); }

        Action action = actionTable[{states.back(), src[i]}];
        switch (action.type) {
            case Action::Type::kAction: {
                if (showStates) {
                    cout << fmt::format("ACTION push s{}", action.tgt);
                }
                states.push_back(action.tgt);
                stateNodes.push_back(new GrammarNode({{}, src[i]}));
                i++;
                break;
            }
            case Action::Type::kReduce: {
                Prod                  r = prods[action.tgt];
                vector<GrammarNode *> temp;

                if (showStates) {
                    cout << fmt::format("REDUCE r{} pop ", action.tgt + 1);
                }
                for (int j = 0; j < r.right.size(); j++) {
                    if (showStates) {
                        cout << fmt::format("s{} ", states.back());
                    }
                    temp.push_back(stateNodes.back());
                    states.pop_back();
                    stateNodes.pop_back();
                }

                assert(actionTable.count({states.back(), r.symbol}) != 0);
                Action action2 = actionTable[{states.back(), r.symbol}];
                states.push_back(action2.tgt);
                if (showStates) {
                    cout << fmt::format("GOTO s{}", action2.tgt);
                }

                GrammarNode *nextNode = new GrammarNode({{}, r.symbol});
                while (temp.size()) {
                    nextNode->children.push_back(temp.back());
                    temp.pop_back();
                }
                stateNodes.push_back(nextNode);
                break;
            }
            case Action::Type::kAccept: {
                if (showStates) { cout << "ACCEPT\t\t"; }
                flag = false;
                break;
            }
            default: {
                assert(false);
                break;
            }
        }
        if (showStates) {
            cout << fmt::format("\nstates: [{}], done\n",
                                fmt::join(states, ", "));
            cout << fmt::format(
                "stateNodes: [{}]\n",
                fmt::join(getStateNodesNames(stateNodes), ", "));
        }
    }
    assert(stateNodes.size() == 1);
    return stateNodes.back();
}

void printGrammarTreeNode(GrammarNode *           node,
                          const map<int, string> &symbolNames, ostream &oss,
                          vector<bool> isLast = {}) {
    if (node->children.size() == 1) {
        printGrammarTreeNode(node->children[0], symbolNames, oss, isLast);
        return;
    }
    if (isLast.size() == 0) { fmt::print(" "); }

    for (int i = 0; i < isLast.size(); i++) {
        if (i + 1 == isLast.size()) {
            fmt::print("{}", (isLast[i] ? " └─ " : " ├─ "));
        } else {
            fmt::print("{}", (isLast[i] ? "   " : " │ "));
        }
    }
    if (node->children.size() == 0) {
        fmt::print("{}\n", symbolNames.at(node->symbol).c_str());
    } else {
        fmt::print("{}\n", symbolNames.at(node->symbol).c_str());
    }
    isLast.push_back(false);
    for (auto it = node->children.begin(); it != node->children.end(); it++) {
        if (it + 1 == node->children.end()) { isLast.back() = true; }
        printGrammarTreeNode(*it, symbolNames, oss, isLast);
    }
}

// test first set and follow set
void test1() {
    fmt::print("test first-set and follow-set \n");
    fmt::print("----------------------------- \n");
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

    auto firstSets  = getFirstSets(grammar);
    auto followSets = getFollowSets(grammar, firstSets);
    fmt::print("\n");
}

// test syntax parsing
void test2() {
    fmt::print("test LR(1) analysis procedure \n");
    fmt::print("----------------------------- \n");
    // initialize symbolset and grammar
    Grammar grammar({
        "Q -> A",
        "A -> A + M",
        "A -> M",
        "M -> M * M",
        "M -> ( A )",
        "M -> d",
    });
    string src = "(d+d)*d+(d)";

    fmt::print("> symbol names:\n");
    for (auto[id, str] : grammar.symbolNames) {
        cout << fmt::format("{:d}: {:s}\n", id, str);
    }

    auto firstSets  = getFirstSets(grammar);
    auto followSets = getFollowSets(grammar, firstSets);
    auto lr1Automata = getLR1automata(grammar);
    auto actionTable = getLR1table(grammar, lr1Automata);

    fmt::print("> use LR(1) Action Table to parse: \n");
    fmt::print("> input string: {}\n", src);
    vector<int> tokens = simpleLexicalParser(grammar.symbolNames, src);
    fmt::print("{}\n\n", fmt::join(tokens, ", "));

    GrammarNode *root = simpleSyntaxParser(grammar, actionTable, tokens, true);
    fmt::print("> grammar tree: \n");
    printGrammarTreeNode(root, grammar.symbolNames, cout);

    fmt::print("\n");
}

// test ambiguous grammar: "Dangling-Else"
void test3() {
    fmt::print("test ambiguous grammar: \"Dangling-Else\" \n");
    fmt::print("--------------------------------------- \n");
    // initialize symbolset and grammar
    Grammar grammar({
        "P -> S",
        "S -> i S t S e S",
        "S -> i S t S",
        "S -> D",
    });

    grammar.prodsPriority[1] = -1;
    string src = "iDtiDtDeD";

    cout << grammar.str();
    fmt::print("> symbol names:\n");
    for (auto[id, str] : grammar.symbolNames) {
        cout << fmt::format("{:d}: {:s}\n", id, str);
    }

    auto firstSets  = getFirstSets(grammar);
    auto followSets = getFollowSets(grammar, firstSets);
    auto lr1Automata = getLR1automata(grammar);    
    auto actionTable = getLR1table(grammar, lr1Automata);

    fmt::print("> use LR(1) Action Table to parse: \n");
    fmt::print("> input string: {}\n", src);

    vector<int> tokens = simpleLexicalParser(grammar.symbolNames, src);
    fmt::print("{}\n\n", fmt::join(tokens, ", "));

    GrammarNode *root = simpleSyntaxParser(grammar, actionTable, tokens, true);
    fmt::print("> grammar tree: \n");
    printGrammarTreeNode(root, grammar.symbolNames, cout);
}

// test ambiguous grammar: priority and associacity
void test4() {
    fmt::print("test ambiguous grammar: priority and associacity \n");
    fmt::print("------------------------------------------------ \n");
    Grammar grammar({
        "P -> S",
        "S -> S + S",
        "S -> S - S",
        "S -> S * S",
        "S -> S / S",
        "S -> S < S",
        "S -> S > S",
        "S -> D",
    });

    grammar.prodsPriority[1] = -1;
    grammar.prodsPriority[2] = -1;
    grammar.prodsPriority[3] = -2;
    grammar.prodsPriority[4] = -2;
    grammar.prodsPriority[5] = -3;
    grammar.prodsPriority[6] = -3;
    grammar.prodsAssociate[1] = Associate::kLeft;
    grammar.prodsAssociate[2] = Associate::kLeft;
    grammar.prodsAssociate[3] = Associate::kLeft;
    grammar.prodsAssociate[4] = Associate::kLeft;
    grammar.prodsAssociate[5] = Associate::kRight;
    grammar.prodsAssociate[6] = Associate::kRight;

    string src = "D+D*D<D+D<D+D*D*D*D+D<D";

    cout << grammar.str();
    fmt::print("> symbol names:\n");
    for (auto[id, str] : grammar.symbolNames) {
        cout << fmt::format("{:d}: {:s}\n", id, str);
    }

    auto firstSets  = getFirstSets(grammar);
    auto followSets = getFollowSets(grammar, firstSets);
    auto lr1Automata = getLR1automata(grammar);
    auto actionTable = getLR1table(grammar, lr1Automata);
    
    fmt::print("> use LR(1) Action Table to parse: \n");
    fmt::print("> input string: {}\n", src);
    vector<int> tokens = simpleLexicalParser(grammar.symbolNames, src);
    fmt::print("{}\n\n", fmt::join(tokens, ", "));

    GrammarNode *root = simpleSyntaxParser(grammar, actionTable, tokens, true);
    fmt::print("> grammar tree: \n");
    printGrammarTreeNode(root, grammar.symbolNames, cout);
}

// LALR test
void test5() {
    fmt::print("test empty production in LR(1) grammar \n");
    fmt::print("-------------------------------------- \n");
    // initialize symbolset and grammar
    Grammar grammar({
        "S -> P",
        "P -> ( As Bs )",
        "As -> As A", // 左递归
        "As ->",
        "Bs -> Bs B", // 左递归
        "Bs ->",
    });

    auto lr1Automata = getLR1automata(grammar);
    auto lalr1Automata = getLALR1fromLR1(grammar, lr1Automata);
    auto actionTable = getLR1table(grammar, lalr1Automata);

    fmt::print("> use LR(1) Action Table to analyze: \n");
    string src = "()";
    fmt::print("> input string: {}\n", src);

    vector<int> tokens = simpleLexicalParser(grammar.symbolNames, src);
    cout << src << "\n";

    GrammarNode *root = simpleSyntaxParser(grammar, actionTable, tokens, true);
    fmt::print("> grammar tree: \n");
    printGrammarTreeNode(root, grammar.symbolNames, cout);
}

int main() {
    krill::log::sink_cerr->set_level(spdlog::level::debug);
    vector<void (*)()> testFuncs = {test1, test2, test3, test4, test5};
    // vector<void (*)()> testFuncs = { test5};
    for (int i = 0; i < testFuncs.size(); i++) {
        cout << "#test " << (i + 1) << "\n";
        testFuncs[i]();
        cout << "\n" << "\n";
    }
    return 0;
}
