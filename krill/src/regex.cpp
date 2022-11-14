#include "krill/regex.h"
#include "fmt/format.h"
#include "krill/automata.h"
#include "krill/defs.h"
#include "krill/grammar.h"
#include "krill/utils.h"
#include <algorithm>
#include <cassert>
#include <map>
#include <set>
#include <stack>
#include <vector>


using krill::grammar::Token;
using namespace std;
using namespace krill::automata;
using namespace krill::utils;

namespace krill::regex {

DFA getDFAfromRegex(string src) {
    vector<Token> tokens = core::lexicalParser(src);
    NFA           nfa    = core::syntaxParser(tokens);
    DFA           dfa    = getMinimizedDfa(getDFAfromNFA(nfa));
    return dfa;
}

NFA getNFAfromRegex(string src) {
    vector<Token> tokens = core::lexicalParser(src);
    NFA           nfa    = core::syntaxParser(tokens);
    return nfa;
}
} // namespace krill::regex

namespace krill::regex::core {

char lexValueToChar(string lexValue) {
    if (lexValue == "\n") {
        return '\n';
    } else if (lexValue == "\r") {
        return '\r';
    } else if (lexValue == "\t") {
        return '\t';
    } else if (lexValue == "\v") {
        return '\v';
    } else {
        assert(lexValue.size() == 1);
        return lexValue[0];
    }
}

// RegEx tokens => RegEx NFA
NFA syntaxParser(vector<Token> tokens) {
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
    using ActionTable = map<pair<int, int>, Action>;
    struct Node {
        int       tokenId;
        string    lexValue;
        int       nfaSt;
        int       nfaEd;
        set<char> rangeChars;
        Node     *child;
    };

    // Symbol Names
    static const map<int, string> symbolNames = {
        {-1, "END_"},     {0, "RegEx"}, {1, "Parallel"}, {2, "'|'"},
        {3, "Seq"},       {4, "Item"},  {5, "Closure"},  {6, "Atom"},
        {7, "'+'"},       {8, "'*'"},   {9, "'?'"},      {10, "'('"},
        {11, "')'"},      {12, "Char"}, {13, "Range"},   {14, "'['"},
        {15, "RangeSeq"}, {16, "']'"},  {17, "'^'"},     {18, "RangeItem"},
        {19, "'-'"},      {20, "'.'"},
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
        /* 12: Atom -> Range */ {6, {13}},
        /* 13: Range -> '[' RangeSeq ']' */ {13, {14, 15, 16}},
        /* 14: Range -> '[' '^' RangeSeq ']' */ {13, {14, 17, 15, 16}},
        /* 15: RangeSeq -> RangeSeq RangeItem */ {15, {15, 18}},
        /* 16: RangeSeq -> RangeItem */ {15, {18}},
        /* 17: RangeItem -> Char '-' Char */ {18, {12, 19, 12}},
        /* 18: RangeItem -> Char */ {18, {12}},
        /* 19: Atom -> '.' */ {6, {20}},
    };
    // Action Table
    static const ActionTable actionTable = {
        {{0, 1}, {GOTO, 1}},      {{0, 3}, {GOTO, 2}},
        {{0, 4}, {GOTO, 3}},      {{0, 5}, {GOTO, 4}},
        {{0, 6}, {GOTO, 5}},      {{0, 10}, {ACTION, 6}},
        {{0, 12}, {ACTION, 7}},   {{0, 13}, {GOTO, 8}},
        {{0, 14}, {ACTION, 9}},   {{0, 20}, {ACTION, 10}},
        {{1, -1}, {ACCEPT, 0}},   {{1, 2}, {ACTION, 11}},
        {{2, -1}, {REDUCE, 2}},   {{2, 2}, {REDUCE, 2}},
        {{2, 4}, {GOTO, 12}},     {{2, 5}, {GOTO, 4}},
        {{2, 6}, {GOTO, 5}},      {{2, 10}, {ACTION, 6}},
        {{2, 11}, {REDUCE, 2}},   {{2, 12}, {ACTION, 7}},
        {{2, 13}, {GOTO, 8}},     {{2, 14}, {ACTION, 9}},
        {{2, 20}, {ACTION, 10}},  {{3, -1}, {REDUCE, 4}},
        {{3, 2}, {REDUCE, 4}},    {{3, 10}, {REDUCE, 4}},
        {{3, 11}, {REDUCE, 4}},   {{3, 12}, {REDUCE, 4}},
        {{3, 14}, {REDUCE, 4}},   {{3, 20}, {REDUCE, 4}},
        {{4, -1}, {REDUCE, 5}},   {{4, 2}, {REDUCE, 5}},
        {{4, 10}, {REDUCE, 5}},   {{4, 11}, {REDUCE, 5}},
        {{4, 12}, {REDUCE, 5}},   {{4, 14}, {REDUCE, 5}},
        {{4, 20}, {REDUCE, 5}},   {{5, -1}, {REDUCE, 6}},
        {{5, 2}, {REDUCE, 6}},    {{5, 7}, {ACTION, 13}},
        {{5, 8}, {ACTION, 14}},   {{5, 9}, {ACTION, 15}},
        {{5, 10}, {REDUCE, 6}},   {{5, 11}, {REDUCE, 6}},
        {{5, 12}, {REDUCE, 6}},   {{5, 14}, {REDUCE, 6}},
        {{5, 20}, {REDUCE, 6}},   {{6, 1}, {GOTO, 16}},
        {{6, 3}, {GOTO, 2}},      {{6, 4}, {GOTO, 3}},
        {{6, 5}, {GOTO, 4}},      {{6, 6}, {GOTO, 5}},
        {{6, 10}, {ACTION, 6}},   {{6, 12}, {ACTION, 7}},
        {{6, 13}, {GOTO, 8}},     {{6, 14}, {ACTION, 9}},
        {{6, 20}, {ACTION, 10}},  {{7, -1}, {REDUCE, 11}},
        {{7, 2}, {REDUCE, 11}},   {{7, 7}, {REDUCE, 11}},
        {{7, 8}, {REDUCE, 11}},   {{7, 9}, {REDUCE, 11}},
        {{7, 10}, {REDUCE, 11}},  {{7, 11}, {REDUCE, 11}},
        {{7, 12}, {REDUCE, 11}},  {{7, 14}, {REDUCE, 11}},
        {{7, 20}, {REDUCE, 11}},  {{8, -1}, {REDUCE, 12}},
        {{8, 2}, {REDUCE, 12}},   {{8, 7}, {REDUCE, 12}},
        {{8, 8}, {REDUCE, 12}},   {{8, 9}, {REDUCE, 12}},
        {{8, 10}, {REDUCE, 12}},  {{8, 11}, {REDUCE, 12}},
        {{8, 12}, {REDUCE, 12}},  {{8, 14}, {REDUCE, 12}},
        {{8, 20}, {REDUCE, 12}},  {{9, 12}, {ACTION, 17}},
        {{9, 15}, {GOTO, 18}},    {{9, 17}, {ACTION, 19}},
        {{9, 18}, {GOTO, 20}},    {{10, -1}, {REDUCE, 19}},
        {{10, 2}, {REDUCE, 19}},  {{10, 7}, {REDUCE, 19}},
        {{10, 8}, {REDUCE, 19}},  {{10, 9}, {REDUCE, 19}},
        {{10, 10}, {REDUCE, 19}}, {{10, 11}, {REDUCE, 19}},
        {{10, 12}, {REDUCE, 19}}, {{10, 14}, {REDUCE, 19}},
        {{10, 20}, {REDUCE, 19}}, {{11, 3}, {GOTO, 21}},
        {{11, 4}, {GOTO, 3}},     {{11, 5}, {GOTO, 4}},
        {{11, 6}, {GOTO, 5}},     {{11, 10}, {ACTION, 6}},
        {{11, 12}, {ACTION, 7}},  {{11, 13}, {GOTO, 8}},
        {{11, 14}, {ACTION, 9}},  {{11, 20}, {ACTION, 10}},
        {{12, -1}, {REDUCE, 3}},  {{12, 2}, {REDUCE, 3}},
        {{12, 10}, {REDUCE, 3}},  {{12, 11}, {REDUCE, 3}},
        {{12, 12}, {REDUCE, 3}},  {{12, 14}, {REDUCE, 3}},
        {{12, 20}, {REDUCE, 3}},  {{13, -1}, {REDUCE, 7}},
        {{13, 2}, {REDUCE, 7}},   {{13, 10}, {REDUCE, 7}},
        {{13, 11}, {REDUCE, 7}},  {{13, 12}, {REDUCE, 7}},
        {{13, 14}, {REDUCE, 7}},  {{13, 20}, {REDUCE, 7}},
        {{14, -1}, {REDUCE, 8}},  {{14, 2}, {REDUCE, 8}},
        {{14, 10}, {REDUCE, 8}},  {{14, 11}, {REDUCE, 8}},
        {{14, 12}, {REDUCE, 8}},  {{14, 14}, {REDUCE, 8}},
        {{14, 20}, {REDUCE, 8}},  {{15, -1}, {REDUCE, 9}},
        {{15, 2}, {REDUCE, 9}},   {{15, 10}, {REDUCE, 9}},
        {{15, 11}, {REDUCE, 9}},  {{15, 12}, {REDUCE, 9}},
        {{15, 14}, {REDUCE, 9}},  {{15, 20}, {REDUCE, 9}},
        {{16, 2}, {ACTION, 11}},  {{16, 11}, {ACTION, 22}},
        {{17, 12}, {REDUCE, 18}}, {{17, 16}, {REDUCE, 18}},
        {{17, 19}, {ACTION, 23}}, {{18, 12}, {ACTION, 17}},
        {{18, 16}, {ACTION, 24}}, {{18, 18}, {GOTO, 25}},
        {{19, 12}, {ACTION, 17}}, {{19, 15}, {GOTO, 26}},
        {{19, 18}, {GOTO, 20}},   {{20, 12}, {REDUCE, 16}},
        {{20, 16}, {REDUCE, 16}}, {{21, -1}, {REDUCE, 1}},
        {{21, 2}, {REDUCE, 1}},   {{21, 4}, {GOTO, 12}},
        {{21, 5}, {GOTO, 4}},     {{21, 6}, {GOTO, 5}},
        {{21, 10}, {ACTION, 6}},  {{21, 11}, {REDUCE, 1}},
        {{21, 12}, {ACTION, 7}},  {{21, 13}, {GOTO, 8}},
        {{21, 14}, {ACTION, 9}},  {{21, 20}, {ACTION, 10}},
        {{22, -1}, {REDUCE, 10}}, {{22, 2}, {REDUCE, 10}},
        {{22, 7}, {REDUCE, 10}},  {{22, 8}, {REDUCE, 10}},
        {{22, 9}, {REDUCE, 10}},  {{22, 10}, {REDUCE, 10}},
        {{22, 11}, {REDUCE, 10}}, {{22, 12}, {REDUCE, 10}},
        {{22, 14}, {REDUCE, 10}}, {{22, 20}, {REDUCE, 10}},
        {{23, 12}, {ACTION, 27}}, {{24, -1}, {REDUCE, 13}},
        {{24, 2}, {REDUCE, 13}},  {{24, 7}, {REDUCE, 13}},
        {{24, 8}, {REDUCE, 13}},  {{24, 9}, {REDUCE, 13}},
        {{24, 10}, {REDUCE, 13}}, {{24, 11}, {REDUCE, 13}},
        {{24, 12}, {REDUCE, 13}}, {{24, 14}, {REDUCE, 13}},
        {{24, 20}, {REDUCE, 13}}, {{25, 12}, {REDUCE, 15}},
        {{25, 16}, {REDUCE, 15}}, {{26, 12}, {ACTION, 17}},
        {{26, 16}, {ACTION, 28}}, {{26, 18}, {GOTO, 25}},
        {{27, 12}, {REDUCE, 17}}, {{27, 16}, {REDUCE, 17}},
        {{28, -1}, {REDUCE, 14}}, {{28, 2}, {REDUCE, 14}},
        {{28, 7}, {REDUCE, 14}},  {{28, 8}, {REDUCE, 14}},
        {{28, 9}, {REDUCE, 14}},  {{28, 10}, {REDUCE, 14}},
        {{28, 11}, {REDUCE, 14}}, {{28, 12}, {REDUCE, 14}},
        {{28, 14}, {REDUCE, 14}}, {{28, 20}, {REDUCE, 14}},
    };

    assert(tokens.size() > 0);
    if (tokens[tokens.size() - 1].id != -1) { tokens.push_back({-1, ""}); }

    stack<int>  states;
    stack<Node> stateNodes;
    states.push(0);

    EdgeTable nfaEdges;        // {{symbol, from, to}}
    int       numNfaNodes = 2; // leave 0 for global start, 1 for global end

    for (int i = 0, accpeted = false; !accpeted;) {
        // look tokens[i].id, state => next_state, action
        assert(actionTable.count({states.top(), tokens[i].id}) != 0);

        Action action = actionTable.at({states.top(), tokens[i].id});
        switch (action.type) {
            case ACTION: {
                states.push(action.tgt);
                // just pass the tokenId and lexValue into node
                Node nextNode = Node({.tokenId  = tokens[i].id,
                                      .lexValue = tokens[i].lexValue,
                                      .nfaSt    = -1,
                                      .nfaEd    = -1});
                stateNodes.push(nextNode);

                // fmt::print("[{} \"{}\"]", symbolNames.at(nextNode.tokenId),
                //            nextNode.lexValue);

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
                        nextNode = Node({.tokenId = prods.at(action.tgt).symbol,
                                         .lexValue = child[0].lexValue,
                                         .nfaSt    = child[0].nfaSt,
                                         .nfaEd    = child[0].nfaEd});
                        break;
                    }
                    case 1: { // Parallel -> Parallel '|' Seq
                        // promise to be safe
                        int from = numNfaNodes++;
                        int to   = numNfaNodes++;
                        nfaEdges.push_back({.symbol = EMPTY_SYMBOL,
                                            .from   = from,
                                            .to     = child[0].nfaSt});
                        nfaEdges.push_back({.symbol = EMPTY_SYMBOL,
                                            .from   = from,
                                            .to     = child[2].nfaSt});
                        nfaEdges.push_back({.symbol = EMPTY_SYMBOL,
                                            .from   = child[0].nfaEd,
                                            .to     = to});
                        nfaEdges.push_back({.symbol = EMPTY_SYMBOL,
                                            .from   = child[2].nfaEd,
                                            .to     = to});
                        nextNode = Node({.tokenId = prods.at(action.tgt).symbol,
                                         .lexValue = child[0].lexValue + "|" +
                                                     child[2].lexValue,
                                         .nfaSt = from,
                                         .nfaEd = to});
                        break;
                    }
                    case 2: { // Parallel -> Seq
                        nextNode = Node({.tokenId = prods.at(action.tgt).symbol,
                                         .lexValue = child[0].lexValue,
                                         .nfaSt    = child[0].nfaSt,
                                         .nfaEd    = child[0].nfaEd});
                        break;
                    }
                    case 3: { // Seq -> Seq Item
                        // promise to be safe
                        nfaEdges.push_back({.symbol = EMPTY_SYMBOL,
                                            .from   = child[0].nfaEd,
                                            .to     = child[1].nfaSt});
                        nextNode = Node(
                            {.tokenId  = prods.at(action.tgt).symbol,
                             .lexValue = child[0].lexValue + child[1].lexValue,
                             .nfaSt    = child[0].nfaSt,
                             .nfaEd    = child[1].nfaEd});
                        break;
                    }
                    case 4: { // Seq -> Item
                        nextNode = Node({.tokenId = prods.at(action.tgt).symbol,
                                         .lexValue = child[0].lexValue,
                                         .nfaSt    = child[0].nfaSt,
                                         .nfaEd    = child[0].nfaEd});
                        break;
                    }
                    case 5: { // Item -> Closure
                        nextNode = Node({.tokenId = prods.at(action.tgt).symbol,
                                         .lexValue = child[0].lexValue,
                                         .nfaSt    = child[0].nfaSt,
                                         .nfaEd    = child[0].nfaEd});
                        break;
                    }
                    case 6: { // Item -> Atom
                        nextNode = Node({.tokenId = prods.at(action.tgt).symbol,
                                         .lexValue = child[0].lexValue,
                                         .nfaSt    = child[0].nfaSt,
                                         .nfaEd    = child[0].nfaEd});
                        break;
                    }
                    case 7: { // Closure -> Atom '+'
                        nfaEdges.push_back({.symbol = EMPTY_SYMBOL,
                                            .from   = child[0].nfaEd,
                                            .to     = child[0].nfaSt});
                        nextNode = Node({.tokenId = prods.at(action.tgt).symbol,
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
                        nextNode = Node({.tokenId = prods.at(action.tgt).symbol,
                                         .lexValue = child[0].lexValue + "*",
                                         .nfaSt    = child[0].nfaSt,
                                         .nfaEd    = child[0].nfaEd});
                        break;
                    }
                    case 9: { // Closure -> Atom '?'
                        nfaEdges.push_back({.symbol = EMPTY_SYMBOL,
                                            .from   = child[0].nfaSt,
                                            .to     = child[0].nfaEd});
                        nextNode = Node({.tokenId = prods.at(action.tgt).symbol,
                                         .lexValue = child[0].lexValue + "?",
                                         .nfaSt    = child[0].nfaSt,
                                         .nfaEd    = child[0].nfaEd});
                        break;
                    }
                    case 10: { // Atom -> '(' Parallel ')'
                        nextNode =
                            Node({.tokenId  = prods.at(action.tgt).symbol,
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
                        nextNode = Node({.tokenId = prods.at(action.tgt).symbol,
                                         .lexValue = child[0].lexValue,
                                         .nfaSt    = from,
                                         .nfaEd    = to});
                        break;
                    }
                    case 12: { // Atom -> Range
                        nextNode = Node({.tokenId = prods.at(action.tgt).symbol,
                                         .lexValue = child[0].lexValue,
                                         .nfaSt    = child[0].nfaSt,
                                         .nfaEd    = child[0].nfaEd});
                        break;
                    }
                    case 13: { // Range -> '[' RangeSeq ']'
                        // promise to be safe
                        int from = numNfaNodes++;
                        int to   = numNfaNodes++;
                        for (char symbol : child[1].rangeChars) {
                            nfaEdges.push_back({symbol, from, to});
                        }
                        int from2 = numNfaNodes++;
                        int to2   = numNfaNodes++;
                        nfaEdges.push_back({EMPTY_SYMBOL, from2, from});
                        nfaEdges.push_back({EMPTY_SYMBOL, to, to2});
                        nextNode =
                            Node({.tokenId  = prods.at(action.tgt).symbol,
                                  .lexValue = "[" + child[1].lexValue + "]",
                                  .nfaSt    = from2,
                                  .nfaEd    = to2});
                        break;
                    }
                    case 14: { // Range -> '[' '^' RangeSeq ']'
                        // promise to be safe
                        int from = numNfaNodes++;
                        int to   = numNfaNodes++;
                        for (int c = 1; c <= 127; c++) {
                            if (child[2].rangeChars.count((char) c) == 0) {
                                nfaEdges.push_back({c, from, to});
                            }
                        }
                        int from2 = numNfaNodes++;
                        int to2   = numNfaNodes++;
                        nfaEdges.push_back({EMPTY_SYMBOL, from2, from});
                        nfaEdges.push_back({EMPTY_SYMBOL, to, to2});
                        nextNode =
                            Node({.tokenId  = prods.at(action.tgt).symbol,
                                  .lexValue = "[^" + child[2].lexValue + "]",
                                  .nfaSt    = from2,
                                  .nfaEd    = to2});
                        break;
                    }
                    case 15: { // RangeSeq -> RangeSeq RangeItem
                        set<char> rangeChars = child[0].rangeChars;
                        rangeChars.insert(child[1].rangeChars.begin(),
                                          child[1].rangeChars.end());
                        nextNode = Node(
                            {.tokenId  = prods.at(action.tgt).symbol,
                             .lexValue = child[0].lexValue + child[1].lexValue,
                             .rangeChars = rangeChars});
                        break;
                    }
                    case 16: { // RangeSeq -> RangeItem
                        nextNode = Node({.tokenId = prods.at(action.tgt).symbol,
                                         .lexValue   = child[0].lexValue,
                                         .rangeChars = child[0].rangeChars});
                        break;
                    }
                    case 17: { // RangeItem -> Char '-' Char
                        char c1 = lexValueToChar(child[0].lexValue);
                        char c2 = lexValueToChar(child[2].lexValue);
                        assert(c2 > c1);
                        set<char> rangeChars;
                        for (char c = c1; c <= c2; c++) {
                            rangeChars.insert(c);
                        }
                        nextNode = Node({.tokenId = prods.at(action.tgt).symbol,
                                         .lexValue = child[0].lexValue + "-" +
                                                     child[2].lexValue,
                                         .rangeChars = rangeChars});
                        break;
                    }
                    case 18: { // RangeItem -> Char
                        char      c = lexValueToChar(child[0].lexValue);
                        set<char> rangeChars = {c};
                        nextNode = Node({.tokenId = prods.at(action.tgt).symbol,
                                         .lexValue   = child[0].lexValue,
                                         .rangeChars = rangeChars});
                        break;
                    }
                    case 19: { // Atom -> '.'
                        // promise to be safe
                        int from = numNfaNodes++;
                        int to   = numNfaNodes++;
                        for (int c = 1; c <= 127; c++) {
                            nfaEdges.push_back({c, from, to});
                        }
                        int from2 = numNfaNodes++;
                        int to2   = numNfaNodes++;
                        nfaEdges.push_back({EMPTY_SYMBOL, from2, from});
                        nfaEdges.push_back({EMPTY_SYMBOL, to, to2});
                        nextNode = Node({.tokenId = prods.at(action.tgt).symbol,
                                         .lexValue = child[0].lexValue,
                                         .nfaSt    = from2,
                                         .nfaEd    = to2});
                        break;
                    }
                    default: {
                        assert(false);
                        break;
                    }
                }

                // fmt::print("[{} \"{}\"]", symbolNames.at(nextNode.tokenId),
                //            nextNode.lexValue);

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

    // fmt::print("\n");
    return NFA({nfaGraph, finality});
}

// RegEx String => tokens
vector<Token> lexicalParser(string src) {
    //   {-1, "END_"}, {0, "RegEx"}, {1, "Parallel"}, {2, "'|'"}, {3, "Seq"},
    //   {4, "Item"}, {5, "Closure"}, {6, "Atom"}, {7, "'+'"}, {8, "'*'"}, {9,
    //   "'?'"}, {10, "'('"}, {11, "')'"}, {12, "Char"}, {13, "Range"}, {14,
    //   "'['"}, {15, "RangeSeq"}, {16, "']'"}, {17, "'^'"}, {18, "RangeItem"},
    //   {19, "'-'"}, {20, "'.'"}
    vector<Token>        tokens;
    const map<char, int> lexMap = {
        {'|', 2},  {'+', 7},  {'*', 8},  {'?', 9},  {'(', 10}, {')', 11},
        {'[', 14}, {']', 16}, {'^', 17}, {'-', 19}, {'.', 20},
    };

    bool isEscape = false;
    for (char c : src) {
        int id;

        if (isEscape) {
            if (c == 'n' || c == 'r' || c == 't' || c == 'v') {
                string s = "\\" + c;
                tokens.push_back({.id = 12, .lexValue = s});
            } else {
                tokens.push_back({.id = 12, .lexValue = string(1, c)});
            }
            isEscape = false;
            continue;
        }

        if (c == '\\') {
            isEscape = true;
            continue;
        } else if (lexMap.count(c) != 0) {
            id = lexMap.at(c);
        } else {
            id = 12;
        }

        tokens.push_back({.id = id, .lexValue = string(1, c)});
    }

    assert(isEscape == false);
    return tokens;
}

} // namespace krill::regex::core
