#include "krill/regex.h"
#include "fmt/format.h"
#include "krill/automata.h"
#include "krill/defs.h"
#include "krill/grammar.h"
#include "krill/utils.h"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <map>
#include <set>
#include <stack>
#include <vector>

#define ERR_LOG(...)                                                           \
    {                                                                          \
        std::cerr << fmt::format("{}:line {}: ", __FILE__, __LINE__);          \
        std::cerr << fmt::format(__VA_ARGS__);                                 \
        std::cerr << "\n";                                                     \
        assert(false);                                                         \
    }

using namespace std;
using namespace krill::automata;
using namespace krill::grammar;
using namespace krill::utils;

namespace krill::regex::core {

// Grammar
#define END_ -1
#define RegEx 256
#define Parallel 257
#define Seq 258
#define Item 259
#define Closure 260
#define Atom 261
#define Char 262
#define Range 263
#define RangeSeq 264
#define RangeItem 265

#define AC0 Action::Type::kAction
#define AC1 Action::Type::kReduce
#define AC2 Action::Type::kGoto
#define AC3 Action::Type::kAccept

// Action Table
const ActionTable actionTable = {
    {{0, 40}, {AC0, 1}},    {{0, 46}, {AC0, 2}},    {{0, 91}, {AC0, 3}},
    {{0, 257}, {AC2, 4}},   {{0, 258}, {AC2, 5}},   {{0, 259}, {AC2, 6}},
    {{0, 260}, {AC2, 7}},   {{0, 261}, {AC2, 8}},   {{0, 262}, {AC0, 9}},
    {{0, 263}, {AC2, 10}},  {{1, 40}, {AC0, 1}},    {{1, 46}, {AC0, 2}},
    {{1, 91}, {AC0, 3}},    {{1, 257}, {AC2, 11}},  {{1, 258}, {AC2, 5}},
    {{1, 259}, {AC2, 6}},   {{1, 260}, {AC2, 7}},   {{1, 261}, {AC2, 8}},
    {{1, 262}, {AC0, 9}},   {{1, 263}, {AC2, 10}},  {{2, -1}, {AC1, 19}},
    {{2, 40}, {AC1, 19}},   {{2, 41}, {AC1, 19}},   {{2, 42}, {AC1, 19}},
    {{2, 43}, {AC1, 19}},   {{2, 46}, {AC1, 19}},   {{2, 63}, {AC1, 19}},
    {{2, 91}, {AC1, 19}},   {{2, 124}, {AC1, 19}},  {{2, 262}, {AC1, 19}},
    {{3, 94}, {AC0, 12}},   {{3, 262}, {AC0, 13}},  {{3, 264}, {AC2, 14}},
    {{3, 265}, {AC2, 15}},  {{4, -1}, {AC3, 0}},    {{4, 124}, {AC0, 16}},
    {{5, -1}, {AC1, 2}},    {{5, 40}, {AC0, 1}},    {{5, 41}, {AC1, 2}},
    {{5, 46}, {AC0, 2}},    {{5, 91}, {AC0, 3}},    {{5, 124}, {AC1, 2}},
    {{5, 259}, {AC2, 17}},  {{5, 260}, {AC2, 7}},   {{5, 261}, {AC2, 8}},
    {{5, 262}, {AC0, 9}},   {{5, 263}, {AC2, 10}},  {{6, -1}, {AC1, 4}},
    {{6, 40}, {AC1, 4}},    {{6, 41}, {AC1, 4}},    {{6, 46}, {AC1, 4}},
    {{6, 91}, {AC1, 4}},    {{6, 124}, {AC1, 4}},   {{6, 262}, {AC1, 4}},
    {{7, -1}, {AC1, 5}},    {{7, 40}, {AC1, 5}},    {{7, 41}, {AC1, 5}},
    {{7, 46}, {AC1, 5}},    {{7, 91}, {AC1, 5}},    {{7, 124}, {AC1, 5}},
    {{7, 262}, {AC1, 5}},   {{8, -1}, {AC1, 6}},    {{8, 40}, {AC1, 6}},
    {{8, 41}, {AC1, 6}},    {{8, 42}, {AC0, 18}},   {{8, 43}, {AC0, 19}},
    {{8, 46}, {AC1, 6}},    {{8, 63}, {AC0, 20}},   {{8, 91}, {AC1, 6}},
    {{8, 124}, {AC1, 6}},   {{8, 262}, {AC1, 6}},   {{9, -1}, {AC1, 11}},
    {{9, 40}, {AC1, 11}},   {{9, 41}, {AC1, 11}},   {{9, 42}, {AC1, 11}},
    {{9, 43}, {AC1, 11}},   {{9, 46}, {AC1, 11}},   {{9, 63}, {AC1, 11}},
    {{9, 91}, {AC1, 11}},   {{9, 124}, {AC1, 11}},  {{9, 262}, {AC1, 11}},
    {{10, -1}, {AC1, 12}},  {{10, 40}, {AC1, 12}},  {{10, 41}, {AC1, 12}},
    {{10, 42}, {AC1, 12}},  {{10, 43}, {AC1, 12}},  {{10, 46}, {AC1, 12}},
    {{10, 63}, {AC1, 12}},  {{10, 91}, {AC1, 12}},  {{10, 124}, {AC1, 12}},
    {{10, 262}, {AC1, 12}}, {{11, 41}, {AC0, 21}},  {{11, 124}, {AC0, 16}},
    {{12, 262}, {AC0, 13}}, {{12, 264}, {AC2, 22}}, {{12, 265}, {AC2, 15}},
    {{13, 45}, {AC0, 23}},  {{13, 93}, {AC1, 18}},  {{13, 262}, {AC1, 18}},
    {{14, 93}, {AC0, 24}},  {{14, 262}, {AC0, 13}}, {{14, 265}, {AC2, 25}},
    {{15, 93}, {AC1, 16}},  {{15, 262}, {AC1, 16}}, {{16, 40}, {AC0, 1}},
    {{16, 46}, {AC0, 2}},   {{16, 91}, {AC0, 3}},   {{16, 258}, {AC2, 26}},
    {{16, 259}, {AC2, 6}},  {{16, 260}, {AC2, 7}},  {{16, 261}, {AC2, 8}},
    {{16, 262}, {AC0, 9}},  {{16, 263}, {AC2, 10}}, {{17, -1}, {AC1, 3}},
    {{17, 40}, {AC1, 3}},   {{17, 41}, {AC1, 3}},   {{17, 46}, {AC1, 3}},
    {{17, 91}, {AC1, 3}},   {{17, 124}, {AC1, 3}},  {{17, 262}, {AC1, 3}},
    {{18, -1}, {AC1, 8}},   {{18, 40}, {AC1, 8}},   {{18, 41}, {AC1, 8}},
    {{18, 46}, {AC1, 8}},   {{18, 91}, {AC1, 8}},   {{18, 124}, {AC1, 8}},
    {{18, 262}, {AC1, 8}},  {{19, -1}, {AC1, 7}},   {{19, 40}, {AC1, 7}},
    {{19, 41}, {AC1, 7}},   {{19, 46}, {AC1, 7}},   {{19, 91}, {AC1, 7}},
    {{19, 124}, {AC1, 7}},  {{19, 262}, {AC1, 7}},  {{20, -1}, {AC1, 9}},
    {{20, 40}, {AC1, 9}},   {{20, 41}, {AC1, 9}},   {{20, 46}, {AC1, 9}},
    {{20, 91}, {AC1, 9}},   {{20, 124}, {AC1, 9}},  {{20, 262}, {AC1, 9}},
    {{21, -1}, {AC1, 10}},  {{21, 40}, {AC1, 10}},  {{21, 41}, {AC1, 10}},
    {{21, 42}, {AC1, 10}},  {{21, 43}, {AC1, 10}},  {{21, 46}, {AC1, 10}},
    {{21, 63}, {AC1, 10}},  {{21, 91}, {AC1, 10}},  {{21, 124}, {AC1, 10}},
    {{21, 262}, {AC1, 10}}, {{22, 93}, {AC0, 27}},  {{22, 262}, {AC0, 13}},
    {{22, 265}, {AC2, 25}}, {{23, 262}, {AC0, 28}}, {{24, -1}, {AC1, 13}},
    {{24, 40}, {AC1, 13}},  {{24, 41}, {AC1, 13}},  {{24, 42}, {AC1, 13}},
    {{24, 43}, {AC1, 13}},  {{24, 46}, {AC1, 13}},  {{24, 63}, {AC1, 13}},
    {{24, 91}, {AC1, 13}},  {{24, 124}, {AC1, 13}}, {{24, 262}, {AC1, 13}},
    {{25, 93}, {AC1, 15}},  {{25, 262}, {AC1, 15}}, {{26, -1}, {AC1, 1}},
    {{26, 40}, {AC0, 1}},   {{26, 41}, {AC1, 1}},   {{26, 46}, {AC0, 2}},
    {{26, 91}, {AC0, 3}},   {{26, 124}, {AC1, 1}},  {{26, 259}, {AC2, 17}},
    {{26, 260}, {AC2, 7}},  {{26, 261}, {AC2, 8}},  {{26, 262}, {AC0, 9}},
    {{26, 263}, {AC2, 10}}, {{27, -1}, {AC1, 14}},  {{27, 40}, {AC1, 14}},
    {{27, 41}, {AC1, 14}},  {{27, 42}, {AC1, 14}},  {{27, 43}, {AC1, 14}},
    {{27, 46}, {AC1, 14}},  {{27, 63}, {AC1, 14}},  {{27, 91}, {AC1, 14}},
    {{27, 124}, {AC1, 14}}, {{27, 262}, {AC1, 14}}, {{28, 93}, {AC1, 17}},
    {{28, 262}, {AC1, 17}},
};

const map<int, string> symbolNames = {
    {-1, "END_"},      {40, "'('"},       {41, "')'"},   {42, "'*'"},
    {43, "'+'"},       {45, "'-'"},       {46, "'.'"},   {63, "'?'"},
    {91, "'['"},       {93, "']'"},       {94, "'^'"},   {124, "'|'"},
    {256, "RegEx"},    {257, "Parallel"}, {258, "Seq"},  {259, "Item"},
    {260, "Closure"},  {261, "Atom"},     {262, "Char"}, {263, "Range"},
    {264, "RangeSeq"}, {265, "RangeItem"}};
const set<int> terminalSet = {40, 41, 42, 43, 45, 46, 63, 91, 93, 94, 124, 262};
const set<int> nonterminalSet = {256, 257, 258, 259, 260, 261, 263, 264, 265};

const vector<Prod> prods = {
    /* 0: RegEx -> Parallel */ {RegEx, {Parallel}},
    /* 1: Parallel -> Parallel '|' Seq */ {Parallel, {Parallel, '|', Seq}},
    /* 2: Parallel -> Seq */ {Parallel, {Seq}},
    /* 3: Seq -> Seq Item */ {Seq, {Seq, Item}},
    /* 4: Seq -> Item */ {Seq, {Item}},
    /* 5: Item -> Closure */ {Item, {Closure}},
    /* 6: Item -> Atom */ {Item, {Atom}},
    /* 7: Closure -> Atom '+' */ {Closure, {Atom, '+'}},
    /* 8: Closure -> Atom '*' */ {Closure, {Atom, '*'}},
    /* 9: Closure -> Atom '?' */ {Closure, {Atom, '?'}},
    /* 10: Atom -> '(' Parallel ')' */ {Atom, {'(', Parallel, ')'}},
    /* 11: Atom -> Char */ {Atom, {Char}},
    /* 12: Atom -> Range */ {Atom, {Range}},
    /* 13: Range -> '[' RangeSeq ']' */ {Range, {'[', RangeSeq, ']'}},
    /* 14: Range -> '[' '^' RangeSeq ']' */ {Range, {'[', '^', RangeSeq, ']'}},
    /* 15: RangeSeq -> RangeSeq RangeItem */ {RangeSeq, {RangeSeq, RangeItem}},
    /* 16: RangeSeq -> RangeItem */ {RangeSeq, {RangeItem}},
    /* 17: RangeItem -> Char '-' Char */ {RangeItem, {Char, '-', Char}},
    /* 18: RangeItem -> Char */ {RangeItem, {Char}},
    /* 19: Atom -> '.' */ {Atom, {'.'}},
};

const Grammar RegexParser::grammar_ =
    Grammar(terminalSet, nonterminalSet, prods, symbolNames);

// RegEx String => tokens
void RegexParser::lexicalParse() {
    static const map<char, int> lexMap = {
        {'|', '|'}, {'+', '+'}, {'*', '*'}, {'?', '?'}, {'(', '('}, {')', ')'},
        {'[', '['}, {']', ']'}, {'^', '^'}, {'-', '-'}, {'.', '.'},
    };
    static const map<char, string> escapeLexMap = {
        {'n', "\n"}, {'r', "\r"}, {'t', "\t"}, {'v', "\v"}, {'f', "\f"}};
    static const map<char, char> escapeRealMap = {
        {'n', '\n'}, {'r', '\r'}, {'t', '\t'}, {'v', '\v'}, {'f', '\f'}};

    tokens_       = {};
    int  pos      = 0;
    bool isEscape = false;
    for (char c : regex_) {
        if (isEscape) {
            if (escapeLexMap.count(c) != 0) {
                tokens_.push_back({.id   = Char,
                                   .lval = escapeLexMap.at(c),
                                   .rval = escapeRealMap.at(c),
                                   .st   = pos - 1,
                                   .ed   = pos});
            } else {
                tokens_.push_back({.id   = Char,
                                   .lval = string("\\") + c,
                                   .rval = c,
                                   .st   = pos - 1,
                                   .ed   = pos});
            }
            isEscape = false;
            continue;
        } else if (c == '\\') {
            isEscape = true;
        } else {
            int id_ = (lexMap.count(c) != 0) ? lexMap.at(c) : Char;
            tokens_.push_back({.id   = id_,
                               .lval = string(1, c),
                               .rval = c,
                               .st   = pos,
                               .ed   = pos});
        }
        pos++;
    }

    // assert(isEscape == false);
    if (isEscape) {
        ERR_LOG("Regex Parsing Error: unmatched escape '\\' in ‘{}’", regex_);
    }
}

// RegEx tokens => RegEx NFA
void RegexParser::syntaxParse() {
    // assert(tokens.size() > 0);
    if (tokens_.size() == 0) {
        ERR_LOG("Regex Parsing Error: do not accpet empty regex ‘{}’",
                regex_);
    }
    if (tokens_[tokens_.size() - 1].id != -1) { tokens_.push_back({-1, ""}); }

    nodes_  = {};
    states_ = {};
    states_.push(0);

    for (int i = 0, accpeted = false; !accpeted;) {
        // look tokens_[i].id, state => next_state, action
        // assert(actionTable.count({states_.top(), tokens_[i].id}) != 0);
        if (actionTable.count({states_.top(), tokens_[i].id}) == 0) {
            if (nodes_.size() == 0) {
                ERR_LOG("Regex Parsing Error: unmatched symbol ‘{}’\n"
                        "‘{}’\n {}",
                        tokens_[i].lval, regex_,
                        string(tokens_[i].ed - tokens_[i].st + 1, '^'));
            } else {
                ERR_LOG("Regex Parsing Error: unmatched symbol ‘{}’\n"
                        "‘{}’\n {}",
                        tokens_[i].lval, regex_,
                        string(nodes_.top().st, ' ') +
                            string(nodes_.top().ed - nodes_.top().st + 1, '~') +
                            string(tokens_[i].ed - tokens_[i].st + 1, '^'));
            }
        }

        Action action = actionTable.at({states_.top(), tokens_[i].id});
        switch (action.type) {
            case Action::Type::kAction: {
                states_.push(action.tgt);
                // just pass the id and lval into node
                Node nextNode;
                nextNode = Node({
                    .id    = tokens_[i].id,
                    .lval  = tokens_[i].lval,
                    .rval  = tokens_[i].rval,
                    .nfaSt = -1,
                    .nfaEd = -1,
                    .st    = tokens_[i].st,
                    .ed    = tokens_[i].ed,
                });
                nodes_.push(nextNode);
                i++;
                break;
            }
            case Action::Type::kReduce: {
                Prod         r = prods[action.tgt];
                vector<Node> child; // child nodes
                for (int j = 0; j < (int) r.right.size(); j++) {
                    states_.pop();
                    child.insert(child.begin(), nodes_.top());
                    nodes_.pop();
                }


                // assert(actionTable.count({states_.top(), r.symbol}) != 0);
                if (actionTable.count({states_.top(), r.symbol}) == 0) {
                    ERR_LOG(
                        "Regex Parsing Error: unmatched symbol ‘{}’\n"
                        "‘{}’\n{}",
                        tokens_[i].lval, regex_,
                        string(' ', nodes_.top().st) +
                            string(' ', nodes_.top().st) +
                            string('~', nodes_.top().ed - nodes_.top().st + 1) +
                            string('^', tokens_[i].ed - tokens_[i].st + 1));
                }
                Action action2 = actionTable.at({states_.top(), r.symbol});
                assert(action2.type == Action::Type::kGoto);
                states_.push(action2.tgt);

                Node nextNode;
                switch (action.tgt) {
                    case 0:   // RegEx -> Parallel
                    case 2:   // Parallel -> Seq
                    case 4:   // Seq -> Item
                    case 5:   // Item -> Closure
                    case 6: { // Item -> Atom
                        nextNode = Node({.id    = prods.at(action.tgt).symbol,
                                         .lval  = child[0].lval,
                                         .nfaSt = child[0].nfaSt,
                                         .nfaEd = child[0].nfaEd,
                                         .st    = child[0].st,
                                         .ed    = child[0].ed});
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
                        nextNode =
                            Node({.id    = prods.at(action.tgt).symbol,
                                  .lval  = child[0].lval + "|" + child[2].lval,
                                  .nfaSt = from,
                                  .nfaEd = to,
                                  .st    = child[0].st,
                                  .ed    = child[2].ed});
                        break;
                    }
                    case 3: { // Seq -> Seq Item
                        // promise to be safe
                        nfaEdges.push_back({.symbol = EMPTY_SYMBOL,
                                            .from   = child[0].nfaEd,
                                            .to     = child[1].nfaSt});
                        nextNode = Node({.id    = prods.at(action.tgt).symbol,
                                         .lval  = child[0].lval + child[1].lval,
                                         .nfaSt = child[0].nfaSt,
                                         .nfaEd = child[1].nfaEd,
                                         .st    = child[0].st,
                                         .ed    = child[1].ed});
                        break;
                    }
                    case 7: { // Closure -> Atom '+'
                        nfaEdges.push_back({.symbol = EMPTY_SYMBOL,
                                            .from   = child[0].nfaEd,
                                            .to     = child[0].nfaSt});
                        nextNode = Node({.id    = prods.at(action.tgt).symbol,
                                         .lval  = child[0].lval + "+",
                                         .nfaSt = child[0].nfaSt,
                                         .nfaEd = child[0].nfaEd,
                                         .st    = child[0].st,
                                         .ed    = child[1].ed});
                        break;
                    }
                    case 8: { // Closure -> Atom '*'
                        nfaEdges.push_back({.symbol = EMPTY_SYMBOL,
                                            .from   = child[0].nfaSt,
                                            .to     = child[0].nfaEd});
                        nfaEdges.push_back({.symbol = EMPTY_SYMBOL,
                                            .from   = child[0].nfaEd,
                                            .to     = child[0].nfaSt});
                        nextNode = Node({.id    = prods.at(action.tgt).symbol,
                                         .lval  = child[0].lval + "*",
                                         .nfaSt = child[0].nfaSt,
                                         .nfaEd = child[0].nfaEd,
                                         .st    = child[0].st,
                                         .ed    = child[1].ed});
                        break;
                    }
                    case 9: { // Closure -> Atom '?'
                        nfaEdges.push_back({.symbol = EMPTY_SYMBOL,
                                            .from   = child[0].nfaSt,
                                            .to     = child[0].nfaEd});
                        nextNode = Node({.id    = prods.at(action.tgt).symbol,
                                         .lval  = child[0].lval + "?",
                                         .nfaSt = child[0].nfaSt,
                                         .nfaEd = child[0].nfaEd,
                                         .st    = child[0].st,
                                         .ed    = child[1].ed});
                        break;
                    }
                    case 10: { // Atom -> '(' Parallel ')'
                        nextNode = Node({.id    = prods.at(action.tgt).symbol,
                                         .lval  = "(" + child[1].lval + ")",
                                         .nfaSt = child[1].nfaSt,
                                         .nfaEd = child[1].nfaEd,
                                         .st    = child[0].st,
                                         .ed    = child[2].ed});
                        break;
                    }
                    case 11: { // Atom -> Char
                        char symbol = child[0].rval;
                        int  from   = numNfaNodes++;
                        int  to     = numNfaNodes++;
                        nfaEdges.push_back({symbol, from, to});
                        nextNode = Node({.id    = prods.at(action.tgt).symbol,
                                         .lval  = child[0].lval,
                                         .nfaSt = from,
                                         .nfaEd = to,
                                         .st    = child[0].st,
                                         .ed    = child[0].ed});
                        break;
                    }
                    case 12: { // Atom -> Range
                        nextNode = Node({.id    = prods.at(action.tgt).symbol,
                                         .lval  = child[0].lval,
                                         .nfaSt = child[0].nfaSt,
                                         .nfaEd = child[0].nfaEd,
                                         .st    = child[0].st,
                                         .ed    = child[0].ed});
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
                        nextNode = Node({.id    = prods.at(action.tgt).symbol,
                                         .lval  = "[" + child[1].lval + "]",
                                         .nfaSt = from2,
                                         .nfaEd = to2,
                                         .st    = child[0].st,
                                         .ed    = child[2].ed});
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
                        nextNode = Node({.id    = prods.at(action.tgt).symbol,
                                         .lval  = "[^" + child[2].lval + "]",
                                         .nfaSt = from2,
                                         .nfaEd = to2,
                                         .st    = child[0].st,
                                         .ed    = child[3].ed});
                        break;
                    }
                    case 15: { // RangeSeq -> RangeSeq RangeItem
                        set<char> rangeChars = child[0].rangeChars;
                        rangeChars.insert(child[1].rangeChars.begin(),
                                          child[1].rangeChars.end());
                        nextNode = Node({.id   = prods.at(action.tgt).symbol,
                                         .lval = child[0].lval + child[1].lval,
                                         .rangeChars = rangeChars,
                                         .st         = child[0].st,
                                         .ed         = child[1].ed});
                        break;
                    }
                    case 16: { // RangeSeq -> RangeItem
                        nextNode = Node({.id   = prods.at(action.tgt).symbol,
                                         .lval = child[0].lval,
                                         .rangeChars = child[0].rangeChars,
                                         .st         = child[0].st,
                                         .ed         = child[0].ed});
                        break;
                    }
                    case 17: { // RangeItem -> Char '-' Char
                        char c1 = child[0].rval;
                        char c2 = child[2].rval;
                        assert(c2 > c1);
                        set<char> rangeChars;
                        for (char c = c1; c <= c2; c++) {
                            rangeChars.insert(c);
                        }
                        nextNode =
                            Node({.id   = prods.at(action.tgt).symbol,
                                  .lval = child[0].lval + "-" + child[2].lval,
                                  .rangeChars = rangeChars,
                                  .st         = child[0].st,
                                  .ed         = child[2].ed});
                        break;
                    }
                    case 18: { // RangeItem -> Char
                        char      c          = child[0].rval;
                        set<char> rangeChars = {c};
                        nextNode = Node({.id   = prods.at(action.tgt).symbol,
                                         .lval = child[0].lval,
                                         .rangeChars = rangeChars,
                                         .st         = child[0].st,
                                         .ed         = child[0].ed});
                        break;
                    }
                    case 19: { // Atom -> '.'
                        // promise to be safe
                        int from = numNfaNodes++;
                        int to   = numNfaNodes++;
                        for (int c = 1; c <= 127; c++) {
                            if (c != '\r' && c != '\n') {
                                nfaEdges.push_back({c, from, to});
                            }
                        }
                        int from2 = numNfaNodes++;
                        int to2   = numNfaNodes++;
                        nfaEdges.push_back({EMPTY_SYMBOL, from2, from});
                        nfaEdges.push_back({EMPTY_SYMBOL, to, to2});
                        nextNode = Node({.id    = prods.at(action.tgt).symbol,
                                         .lval  = child[0].lval,
                                         .nfaSt = from2,
                                         .nfaEd = to2,
                                         .st    = child[0].st,
                                         .ed    = child[0].ed});
                        break;
                    }
                    default: {
                        assert(false);
                        break;
                    }
                }

                // fmt::print("[{} ‘{}’]", symbolNames.at(nextNode.id),
                //            nextNode.lval);

                nodes_.push(nextNode);
                break;
            }
            case Action::Type::kAccept: {
                accpeted = true;
                break;
            }
            default: {
                assert(false);
                break;
            }
        }
    }
    /*
        assert(nodes_.size() == 1);
        Node root = stateNodes.top();
        */
}

RegexParser::RegexParser(string regex) : regex_(regex), numNfaNodes(2) {
    lexicalParse();
    syntaxParse();
}

NFA RegexParser::nfa() {
    Node root = nodes_.top();
    // RegEx NFA: start at 0, terminate at 1
    nfaEdges.push_back({.symbol = EMPTY_SYMBOL, .from = 0, .to = root.nfaSt});
    nfaEdges.push_back({.symbol = EMPTY_SYMBOL, .from = root.nfaEd, .to = 1});
    NFAgraph nfaGraph = krill::automata::toNFAgraph(nfaEdges);

    map<int, int> finality;
    for (int i = 0; i < numNfaNodes; i++) { finality[i] = 0; }
    finality[1] = 1;

    // fmt::print("\n");
    return NFA({nfaGraph, finality});
}

DFA RegexParser::dfa() { return getMinimizedDfa(getDFAfromNFA(nfa())); }

} // namespace krill::regex::core

namespace krill::regex {

DFA getDFAfromRegex(string src) {
    // vector<Token> tokens = core::lexicalParser(src);
    // NFA           nfa    = core::syntaxParser(tokens);
    // DFA           dfa    = getMinimizedDfa(getDFAfromNFA(nfa));
    // return dfa;
    return core::RegexParser(src).dfa();
}

NFA getNFAfromRegex(string src) {
    // vector<Token> tokens = core::lexicalParser(src);
    // NFA           nfa    = core::syntaxParser(tokens);
    // return nfa;
    return core::RegexParser(src).nfa();
}
} // namespace krill::regex
