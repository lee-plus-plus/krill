#ifndef GRAMMAR_H
#define GRAMMAR_H
#include "defs.h"
#include "automata.h"
#include <map>
#include <set>
#include <string>
#include <vector>
using std::pair, std::set, std::map, std::multimap, std::vector;
using std::string;

namespace krill::type {

struct Prod {
    // Production (P -> Ab)
    int         symbol; // left, nonterminal symbol
    vector<int> right;  // mixed with nonterminals and terminals
    Prod() = default;
    Prod(int symbol, vector<int> right) : symbol(symbol), right(right){};
    // to make std::set happy
    bool operator<(const Prod &p) const;
    bool operator==(const Prod &p) const;

    string str(const map<int, string> &symbolNames) const;
};

enum class Associate {kNone = 0, kLeft = 1, kRight = 2};
string to_string(const Associate &associate);

struct Grammar {
    // Grammar {(P -> Ab), (A -> Abc), (A -> b), ...}
    set<int>         terminalSet;
    set<int>         nonterminalSet;
    vector<Prod>     prods;
    map<int, string> symbolNames;

    // ambiguous grammar setting (unnecessary)
    vector<int>       prodsPriority;
    vector<Associate> prodsAssociate;

    Grammar() = default;
    Grammar(set<int> terminalSet, set<int> nonterminalSet, vector<Prod> prods,
            map<int, string> symbolNames, vector<int> prodsPriority = {},
            vector<Associate> prodsAssociate = {}); // for codegen
    // Grammar(vector<vector<string>> prodStrs); // for quick use
    // Grammar(vector<string> prodStrs); // for quick use

    Grammar(vector<vector<string>> prodSymbolStrs,
            map<string, int>       symbolIds       = {},
            map<string, int>       symbolPriority  = {},
            map<string, Associate> symbolAssociate = {});
    Grammar(vector<string> prodStrs);
    string str() const;

};

enum ActionType { ACTION = 0, REDUCE = 1, GOTO = 2, ACCEPT = 3 };
struct Action {
    // Action of LR1 parse (ACTION | GOTO | REDUCE | ACCEPT, tgt)
    ActionType type; // action type
    int        tgt;  // ACTION or GOTO: tgt=next_state; REDUCE: tgt=prod_idx

    string str() const;
};
// {current_state, look_over_symbol, action}
using ActionTable = map<pair<int, int>, Action>;

string to_string(const ActionTable &tbl, const map<int, string> &symbolNames);

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

    string str(const map<int, string> &symbolNames) const;
};

// LR1 Production Item (P -> A·b, a)
struct ProdLR1Item : ProdItem {
    int search; // search symbol (terminal)
    ProdLR1Item(int symbol, vector<int> right, int dot, int search)
        : ProdItem(symbol, right, dot), search(search){};
    // to make std::set happy
    bool operator<(const ProdLR1Item &p) const;
    bool operator==(const ProdLR1Item &p) const;

    string str(const map<int, string> &symbolNames) const;
};

// Set of LR1 Production Item {(P -> A·b, a), (S -> Sb·, b), }
using LR1State = set<ProdLR1Item>;
string to_string(const LR1State &state, const map<int, string> &symbolNames);

// LR1 Automata (LR1 states, EdgeTable)
struct LR1Automata {
    vector<LR1State> states;
    EdgeTable        edgeTable;

    string str(const map<int, string> &symbolNames) const;
};

map<int, set<int>> getFirstSets(Grammar grammar);
map<int, set<int>> getFollowSets(Grammar grammar, map<int, set<int>> firstSets);

void setLR1StateExpanded(LR1State &states, map<int, set<int>> followSets,
                         Grammar grammar);

LR1Automata getLR1Automata(Grammar grammar);
ActionTable getLR1table(Grammar grammar, LR1Automata lr1Automata);
LR1Automata getLALR1fromLR1(Grammar grammar, LR1Automata lr1Automata);

} // namespace krill::grammar

#endif