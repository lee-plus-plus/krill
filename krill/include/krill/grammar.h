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
};

enum class Associate {kNone = 0, kLeft = 1, kRight = 2};

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

struct Action {
    // Action of LR1 parse (ACTION | GOTO | REDUCE | ACCEPT, tgt)
    enum Type { kAction = 0, kReduce = 1, kGoto = 2, kAccept = 3 };
    Type type; // action type
    int  tgt;  // ACTION or GOTO: tgt=next_state; REDUCE: tgt=prod_idx

    string str() const;
};
// {current_state, look_over_symbol, action}
using ActionTable = map<pair<int, int>, Action>;

string to_string(const Prod &prod, const Grammar &grammar);
string to_string(const EdgeTable &tbl, const Grammar &grammar);
string to_string(const ActionTable &tbl, const Grammar &grammar);
string to_string(const Associate &associate);

}; // namespace krill::type


namespace krill::grammar {
using namespace krill::type;

// Grammar => LR1 Automata (LR1 states, EdgeTable) => Action Table
ActionTable getLR1table(Grammar grammar);
ActionTable getLALR1table(Grammar grammar);

// LR1 Production Item (P -> A·b, a)
struct ProdLR1Item {
    int pidx;   // production idx
    int dot;    // position of dot (0 ~ right.size())
    int search; // search symbol (terminal)
    // to make std::set happy
    bool operator<(const ProdLR1Item &p) const;
    bool operator==(const ProdLR1Item &p) const;
};

// Set of LR1 Production Item {(P -> A·b, a), (S -> Sb·, b), }
using LR1State = set<ProdLR1Item>;
// string to_string(const LR1State &state, const map<int, string> &symbolNames);

// LR1 Automata (LR1 states, EdgeTable)
struct LR1Automata {
    vector<LR1State> states;
    EdgeTable        edgeTable;
};

map<int, set<int>> getFirstSets(Grammar grammar);
map<int, set<int>> getFollowSets(Grammar grammar, map<int, set<int>> firstSets);

LR1State getLR1StateExpanded(const LR1State &          states,
                             const map<int, set<int>> &followSets,
                             const Grammar &           grammar);

LR1Automata getLR1automata(Grammar grammar);
ActionTable getLR1table(Grammar grammar, LR1Automata lr1Automata);
LR1Automata getLALR1fromLR1(Grammar grammar, LR1Automata lr1Automata);

string to_string(const map<int, set<int>> firstSets, const Grammar &grammar);
string to_string(const ProdLR1Item &item, const Grammar &grammar);
string to_string(const LR1State &state, const Grammar &grammar);
string to_string(const LR1Automata &lr1Automata, const Grammar &grammar);

} // namespace krill::grammar

#endif