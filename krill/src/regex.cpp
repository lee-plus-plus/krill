#include "krill/regex.h"
#include "krill/automata.h"
#include "krill/defs.h"
#include "krill/utils.h"
#include <cassert>
#include <map>
#include <stack>
#include <vector>
using std::pair, std::vector, std::map, std::stack;
using namespace krill::automata;
using namespace krill::utils;

namespace krill::regex {

DFA getDFAfromRegex(string src) {
    auto[tokens, lexValues] = core::lexicalParser(src);
    NFA nfa                 = core::syntaxParser(tokens, lexValues);
    DFA dfa                 = getDFAfromNFA(nfa);
    return dfa;
}

NFA getNFAfromRegex(string src) {
    auto[tokens, lexValues] = core::lexicalParser(src);
    NFA nfa                 = core::syntaxParser(tokens, lexValues);
    return nfa;
}
} // namespace krill::regex

namespace krill::regex::core {
// RegEx tokens, lexValues => RegEx NFA
NFA syntaxParser(vector<int> tokens, vector<string> lexValues) {
    // declaration of struct
    struct Prod {
        int         symbol;
        vector<int> right;
    };
    enum TYPE { ACTION, REDUCE, GOTO, ACCEPT };
    struct Action {
        TYPE type;
        int  tgt;
    };
    typedef map<pair<int, int>, Action> ActionTable;
    struct Node {
        int    tokenId;
        string lexValue;
        int    nfaSt;
        int    nfaEd;
    };

    // Symbol Names
    static const map<int, string> symbolNames = {
        {0, "RegEx"},   {1, "Parallel"}, {2, "'|'"},   {3, "Seq"}, {4, "Item"},
        {5, "Closure"}, {6, "Atom"},     {7, "'+'"},   {8, "'*'"}, {9, "'?'"},
        {10, "'('"},    {11, "')'"},     {12, "Char"},
    };
    // Productions
    static const vector<Prod> prods = {
        /* 0: RegEx -> Parallel */ {0, {1}},
        /* 1: Parallel -> Parallel '|' Seq */ {1, {1, 2, 3}},
        /* 2: Parallel -> Seq */ {1, {3}},
        /* 3: Seq -> Seq Item */ {3, {3, 4}},
        /* 4: Seq -> Item */ {3, {4}},
        /* 5: Item -> Closure */ {4, {5}},
        /* 6: Item -> Atom */ {4, {6}},
        /* 7: Closure -> Atom '+' */ {5, {6, 7}},
        /* 8: Closure -> Atom '*' */ {5, {6, 8}},
        /* 9: Closure -> Atom '?' */ {5, {6, 9}},
        /* 10: Atom -> '(' Parallel ')' */ {6, {10, 1, 11}},
        /* 11: Atom -> Char */ {6, {12}},
    };
    // Action Table
    static const ActionTable actionTable = {
        {{0, 1}, {GOTO, 1}},      {{0, 3}, {GOTO, 2}},
        {{0, 4}, {GOTO, 3}},      {{0, 5}, {GOTO, 4}},
        {{0, 6}, {GOTO, 5}},      {{0, 10}, {ACTION, 6}},
        {{0, 12}, {ACTION, 7}},   {{1, -1}, {ACCEPT, 0}},
        {{1, 2}, {ACTION, 8}},    {{2, -1}, {REDUCE, 2}},
        {{2, 2}, {REDUCE, 2}},    {{2, 4}, {GOTO, 9}},
        {{2, 5}, {GOTO, 4}},      {{2, 6}, {GOTO, 5}},
        {{2, 10}, {ACTION, 6}},   {{2, 11}, {REDUCE, 2}},
        {{2, 12}, {ACTION, 7}},   {{3, -1}, {REDUCE, 4}},
        {{3, 2}, {REDUCE, 4}},    {{3, 10}, {REDUCE, 4}},
        {{3, 11}, {REDUCE, 4}},   {{3, 12}, {REDUCE, 4}},
        {{4, -1}, {REDUCE, 5}},   {{4, 2}, {REDUCE, 5}},
        {{4, 10}, {REDUCE, 5}},   {{4, 11}, {REDUCE, 5}},
        {{4, 12}, {REDUCE, 5}},   {{5, -1}, {REDUCE, 6}},
        {{5, 2}, {REDUCE, 6}},    {{5, 7}, {ACTION, 10}},
        {{5, 8}, {ACTION, 11}},   {{5, 9}, {ACTION, 12}},
        {{5, 10}, {REDUCE, 6}},   {{5, 11}, {REDUCE, 6}},
        {{5, 12}, {REDUCE, 6}},   {{6, 1}, {GOTO, 13}},
        {{6, 3}, {GOTO, 2}},      {{6, 4}, {GOTO, 3}},
        {{6, 5}, {GOTO, 4}},      {{6, 6}, {GOTO, 5}},
        {{6, 10}, {ACTION, 6}},   {{6, 12}, {ACTION, 7}},
        {{7, -1}, {REDUCE, 11}},  {{7, 2}, {REDUCE, 11}},
        {{7, 7}, {REDUCE, 11}},   {{7, 8}, {REDUCE, 11}},
        {{7, 9}, {REDUCE, 11}},   {{7, 10}, {REDUCE, 11}},
        {{7, 11}, {REDUCE, 11}},  {{7, 12}, {REDUCE, 11}},
        {{8, 3}, {GOTO, 14}},     {{8, 4}, {GOTO, 3}},
        {{8, 5}, {GOTO, 4}},      {{8, 6}, {GOTO, 5}},
        {{8, 10}, {ACTION, 6}},   {{8, 12}, {ACTION, 7}},
        {{9, -1}, {REDUCE, 3}},   {{9, 2}, {REDUCE, 3}},
        {{9, 10}, {REDUCE, 3}},   {{9, 11}, {REDUCE, 3}},
        {{9, 12}, {REDUCE, 3}},   {{10, -1}, {REDUCE, 7}},
        {{10, 2}, {REDUCE, 7}},   {{10, 10}, {REDUCE, 7}},
        {{10, 11}, {REDUCE, 7}},  {{10, 12}, {REDUCE, 7}},
        {{11, -1}, {REDUCE, 8}},  {{11, 2}, {REDUCE, 8}},
        {{11, 10}, {REDUCE, 8}},  {{11, 11}, {REDUCE, 8}},
        {{11, 12}, {REDUCE, 8}},  {{12, -1}, {REDUCE, 9}},
        {{12, 2}, {REDUCE, 9}},   {{12, 10}, {REDUCE, 9}},
        {{12, 11}, {REDUCE, 9}},  {{12, 12}, {REDUCE, 9}},
        {{13, 2}, {ACTION, 8}},   {{13, 11}, {ACTION, 15}},
        {{14, -1}, {REDUCE, 1}},  {{14, 2}, {REDUCE, 1}},
        {{14, 4}, {GOTO, 9}},     {{14, 5}, {GOTO, 4}},
        {{14, 6}, {GOTO, 5}},     {{14, 10}, {ACTION, 6}},
        {{14, 11}, {REDUCE, 1}},  {{14, 12}, {ACTION, 7}},
        {{15, -1}, {REDUCE, 10}}, {{15, 2}, {REDUCE, 10}},
        {{15, 7}, {REDUCE, 10}},  {{15, 8}, {REDUCE, 10}},
        {{15, 9}, {REDUCE, 10}},  {{15, 10}, {REDUCE, 10}},
        {{15, 11}, {REDUCE, 10}}, {{15, 12}, {REDUCE, 10}},
    };

    if (tokens[tokens.size() - 1] != -1) { tokens.push_back(-1); }

    stack<int>  states;
    stack<Node> stateNodes;
    states.push(0);

    EdgeTable nfaEdges;        // {{symbol, from, to}}
    int       numNfaNodes = 2; // leave 0 for global start, 1 for global end

    for (int i = 0, accpeted = false; !accpeted;) {
        // look tokens[i], state => next_state, action
        assert(actionTable.count({states.top(), tokens[i]}) != 0);

        Action action = actionTable.at({states.top(), tokens[i]});
        switch (action.type) {
            case ACTION: {
                states.push(action.tgt);
                // just pass the tokenId and lexValue into node
                stateNodes.push(Node({.tokenId  = tokens[i],
                                      .lexValue = lexValues[i],
                                      .nfaSt    = -1,
                                      .nfaEd    = -1}));
                i++;
                break;
            }
            case REDUCE: {
                Prod         r = prods[action.tgt];
                vector<Node> child; // child nodes
                for (int j = 0; j < (int) r.right.size(); j++) {
                    states.pop();
                    child.insert(child.begin(), stateNodes.top());
                    stateNodes.pop();
                }

                assert(actionTable.count({states.top(), r.symbol}) != 0);
                Action action2 = actionTable.at({states.top(), r.symbol});
                assert(action2.type == GOTO);
                states.push(action2.tgt);

                Node nextNode;
                switch (action.tgt) {
                    case 0: { // RegEx -> Parallel
                        nextNode = Node({.tokenId  = action.tgt,
                                         .lexValue = child[0].lexValue,
                                         .nfaSt    = child[0].nfaSt,
                                         .nfaEd    = child[0].nfaEd});
                        break;
                    }
                    case 1: { // Parallel -> Parallel '|' Seq
                        nfaEdges.push_back({.symbol = EMPTY_SYMBOL,
                                            .from   = child[0].nfaSt,
                                            .to     = child[2].nfaSt});
                        nfaEdges.push_back({.symbol = EMPTY_SYMBOL,
                                            .from   = child[2].nfaEd,
                                            .to     = child[0].nfaEd});
                        nextNode = Node({.tokenId  = action.tgt,
                                         .lexValue = child[0].lexValue + "|" +
                                                     child[0].lexValue,
                                         .nfaSt = child[0].nfaSt,
                                         .nfaEd = child[0].nfaEd});
                        break;
                    }
                    case 2: { // Parallel -> Seq
                        nextNode = Node({.tokenId  = action.tgt,
                                         .lexValue = child[0].lexValue,
                                         .nfaSt    = child[0].nfaSt,
                                         .nfaEd    = child[0].nfaEd});
                        break;
                    }
                    case 3: { // Seq -> Seq Item
                        nfaEdges.push_back({.symbol = EMPTY_SYMBOL,
                                            .from   = child[0].nfaEd,
                                            .to     = child[1].nfaSt});
                        nextNode = Node({.tokenId  = action.tgt,
                                         .lexValue = child[0].lexValue,
                                         .nfaSt    = child[0].nfaSt,
                                         .nfaEd    = child[1].nfaEd});
                        break;
                    }
                    case 4: { // Seq -> Item
                        nextNode = Node({.tokenId  = action.tgt,
                                         .lexValue = child[0].lexValue,
                                         .nfaSt    = child[0].nfaSt,
                                         .nfaEd    = child[0].nfaEd});
                        break;
                    }
                    case 5: { // Item -> Closure
                        nextNode = Node({.tokenId  = action.tgt,
                                         .lexValue = child[0].lexValue,
                                         .nfaSt    = child[0].nfaSt,
                                         .nfaEd    = child[0].nfaEd});
                        break;
                    }
                    case 6: { // Item -> Atom
                        nextNode = Node({.tokenId  = action.tgt,
                                         .lexValue = child[0].lexValue,
                                         .nfaSt    = child[0].nfaSt,
                                         .nfaEd    = child[0].nfaEd});
                        break;
                    }
                    case 7: { // Closure -> Atom '+'
                        nfaEdges.push_back({.symbol = EMPTY_SYMBOL,
                                            .from   = child[0].nfaEd,
                                            .to     = child[0].nfaSt});
                        nextNode = Node({.tokenId  = action.tgt,
                                         .lexValue = child[0].lexValue + "+",
                                         .nfaSt    = child[0].nfaSt,
                                         .nfaEd    = child[0].nfaEd});
                        break;
                    }
                    case 8: { // Closure -> Atom '*'
                        nfaEdges.push_back({.symbol = EMPTY_SYMBOL,
                                            .from   = child[0].nfaSt,
                                            .to     = child[0].nfaEd});
                        nfaEdges.push_back({.symbol = EMPTY_SYMBOL,
                                            .from   = child[0].nfaEd,
                                            .to     = child[0].nfaSt});
                        nextNode = Node({.tokenId  = action.tgt,
                                         .lexValue = child[0].lexValue + "*",
                                         .nfaSt    = child[0].nfaSt,
                                         .nfaEd    = child[0].nfaEd});
                        break;
                    }
                    case 9: { // Closure -> Atom '?'
                        nfaEdges.push_back({.symbol = EMPTY_SYMBOL,
                                            .from   = child[0].nfaSt,
                                            .to     = child[0].nfaEd});
                        nextNode = Node({.tokenId  = action.tgt,
                                         .lexValue = child[0].lexValue + "?",
                                         .nfaSt    = child[0].nfaSt,
                                         .nfaEd    = child[0].nfaEd});
                        break;
                    }
                    case 10: { // Atom -> '(' Parallel ')'
                        nextNode =
                            Node({.tokenId  = action.tgt,
                                  .lexValue = "(" + child[1].lexValue + ")",
                                  .nfaSt    = child[1].nfaSt,
                                  .nfaEd    = child[1].nfaEd});
                        break;
                    }
                    case 11: { // Atom -> Char
                        char symbol = child[0].lexValue[0];
                        int  from   = numNfaNodes++;
                        int  to     = numNfaNodes++;
                        nfaEdges.push_back({symbol, from, to});
                        nextNode = Node({.tokenId  = action.tgt,
                                         .lexValue = child[0].lexValue,
                                         .nfaSt    = from,
                                         .nfaEd    = to});
                        break;
                    }
                    default: {
                        assert(false);
                        break;
                    }
                }
                stateNodes.push(nextNode);
                break;
            }
            case ACCEPT: {
                accpeted = true;
                break;
            }
            default: {
                assert(false);
                break;
            }
        }
    }
    assert(stateNodes.size() == 1);
    Node root = stateNodes.top();

    // RegEx NFA: start at 0, terminate at 1
    nfaEdges.push_back({.symbol = EMPTY_SYMBOL, .from = 0, .to = root.nfaSt});
    nfaEdges.push_back({.symbol = EMPTY_SYMBOL, .from = root.nfaEd, .to = 1});
    NFAgraph nfaGraph = krill::automata::core::toNFAgraph(nfaEdges);

    map<int, int> finality;
    for (int i = 0; i < numNfaNodes; i++) { finality[i] = 0; }
    finality[1] = 1;

    return NFA({nfaGraph, finality});
}

// RegEx String => tokens, lexValues
pair<vector<int>, vector<string>> lexicalParser(string src) {
    // { {RegEx, "0"}, {Parallel, "1"}, {'|', "2"}, {Seq, "3"}, {Item, "4"},
    //   {Closure, "5"}, {Atom, "6"}, {'+', "7"}, {'*', "8"}, {'?', "9"}, {'(',
    //   "10"}, {')', "11"}, {Char, "12"},}
    vector<int>          tokens;
    vector<string>       lexValues;
    const map<char, int> lexMap = {
        {'|', 2}, {'+', 7}, {'*', 8}, {'?', 9}, {'(', 10}, {')', 11},
    };

    bool isEscape = false;
    for (char c : src) {
        int  token;
        char lexValue;

        if (isEscape) {
            tokens.push_back(token);
            lexValues.push_back(string(1, c));
            continue;
        }

        if (c == '\\') {
            isEscape = true;
            continue;
        } else if (lexMap.count(c) != 0) {
            token = lexMap.at(c);
        } else {
            token = 12;
        }

        tokens.push_back(token);
        lexValues.push_back(string(1, c));
    }

    assert(isEscape == false);
    return make_pair(tokens, lexValues);
}

} // namespace krill::regex::core
