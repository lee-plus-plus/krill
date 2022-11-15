#ifndef GRAMMAR_H
#define GRAMMAR_H
#include "automata.h"
#include "defs.h"
#include <map>
#include <set>
#include <string>
#include <vector>
using std::pair, std::set, std::map, std::multimap, std::vector;
using std::string;

namespace krill::type {

const int END_SYMBOL = -1; // end of input in syntax parsing

struct Prod {
    // Production (P -> Ab)
    int         symbol; // left, nonterminal symbol
    vector<int> right;  // mixed with nonterminals and terminals
    Prod(int symbol, vector<int> right) : symbol(symbol), right(right){};
    // to make std::set happy
    bool operator<(const Prod &p) const;
    bool operator==(const Prod &p) const;
};

struct Grammar {
    // Grammar {(P -> Ab), (A -> Abc), (A -> b), ...}
    set<int>         terminalSet;
    set<int>         nonterminalSet;
    vector<Prod>     prods;
    map<int, string> symbolNames;
    Grammar() = default;
    Grammar(set<int> terminalSet, set<int> nonterminalSet, vector<Prod> prods,
            map<int, string> symbolNames);
    Grammar(vector<Prod> prods);
    Grammar(vector<string> prodStrs);
};

enum ActionType { ACTION = 0, REDUCE = 1, GOTO = 2, ACCEPT = 3 };
struct Action {
    // Action of LR1 parse (ACTION | GOTO | REDUCE | ACCEPT, tgt)
    ActionType type; // action type
    int        tgt;  // ACTION or GOTO: tgt=next_state; REDUCE: tgt=prod_idx
};
// {current_state, look_over_symbol, action}
using ActionTable = map<pair<int, int>, Action>;

struct Token {
    // lexical token
    int    id;
    string lval;
    // to make std::set happy
    bool operator<(const Token &t) const;
    bool operator==(const Token &t) const;
    bool operator!=(const Token &t) const;
};
const Token END_TOKEN = Token({.id = END_SYMBOL, .lval = ""});

}; // namespace krill::type



namespace krill::grammar {
using namespace krill::type;

// Grammar => LR1 Automata (LR1 states, EdgeTable) => Action Table
ActionTable getLR1table(Grammar grammar);
ActionTable getLALR1table(Grammar grammar);

// Production Item (P -> A·b)
struct ProdItem : Prod {
    int dot; // position of dot (0 ~ right.size())
    ProdItem(int symbol, vector<int> right, int dot)
        : Prod(symbol, right), dot(dot){};
    // to make std::set happy
    bool operator<(const ProdItem &p) const;
    bool operator==(const ProdItem &p) const;
};

// LR1 Production Item (P -> A·b, a)
struct ProdLR1Item : ProdItem {
    int search; // search symbol (terminal)
    ProdLR1Item(int symbol, vector<int> right, int dot, int search)
        : ProdItem(symbol, right, dot), search(search){};
    // to make std::set happy
    bool operator<(const ProdLR1Item &p) const;
    bool operator==(const ProdLR1Item &p) const;
};

// Set of LR1 Production Item {(P -> A·b, a), (S -> Sb·, b), }
using LR1State = set<ProdLR1Item>;

// LR1 Automata (LR1 states, EdgeTable)
struct LR1Automata {
    vector<LR1State> states;
    EdgeTable        edgeTable;
};

map<int, set<int>> getFirstSet(Grammar grammar);
map<int, set<int>> getFollowSet(Grammar grammar, map<int, set<int>> firstSet);

void setLR1StateExpanded(LR1State &states, map<int, set<int>> followSet,
                         Grammar grammar);

LR1Automata getLR1Automata(Grammar grammar);
ActionTable getLR1table(Grammar grammar, LR1Automata lr1Automata);
LR1Automata getLALR1fromLR1(Grammar grammar, LR1Automata lr1Automata);

} // namespace krill::grammar

#endif