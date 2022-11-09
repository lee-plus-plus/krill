#ifndef GRAMMAR_H
#define GRAMMAR_H
#include "automata.h"
#include "defs.h"
#include <map>
#include <set>
#include <vector>
using krill::automata::EdgeTable;
using std::pair;
using std::set, std::map, std::multimap, std::vector;

namespace krill::grammar {

const int END_SYMBOL = -1; // 作为语法分析终结标志

// 产生式 (P -> Ab)
struct Prod {
    int symbol;        // left, nonterminal symbol
    vector<int> right; // mixed with nonterminals and terminals
    Prod(int symbol, vector<int> right) : symbol(symbol), right(right){};
    // to make std::set happy
    bool operator<(const Prod &p) const;
    bool operator==(const Prod &p) const;
};

// 文法
struct Grammar {
    set<int> terminalSet;
    set<int> nonterminalSet;
    vector<Prod> prods;
    Grammar(){};
    Grammar(vector<Prod> prods);
};

// 产生式项目 (P -> A·b)
struct ProdItem : Prod {
    int dot; // position of dot (0 ~ right.size())
    ProdItem(int symbol, vector<int> right, int dot)
        : Prod(symbol, right), dot(dot){};
    // to make std::set happy
    bool operator<(const ProdItem &p) const;
    bool operator==(const ProdItem &p) const;
};

// lr(1) 分析表动作
struct Action {
    enum TYPE { ACTION = 0, REDUCE = 1, GOTO = 2, ACCEPT = 3 };
    TYPE type; // 动作类型
    int tgt;   // ACTION or GOTO: tgt=next_state; REDUCE: tgt=prod_idx
};
typedef map<pair<int, int>, Action> ActionTable;


ActionTable getLR1table(Grammar grammar);
ActionTable getLALR1table(Grammar grammar);

}; // namespace krill::grammar

namespace krill::grammar::core {
using namespace krill::grammar;

// LR1产生式项目 (P -> A·b, a)
struct ProdLR1Item : ProdItem {
    int search; // search symbol (terminal)
    ProdLR1Item(int symbol, vector<int> right, int dot, int search)
        : ProdItem(symbol, right, dot), search(search){};
    // to make std::set happy
    bool operator<(const ProdLR1Item &p) const;
    bool operator==(const ProdLR1Item &p) const;
};

// LR1覆盖片
using LR1Cover = set<ProdLR1Item>;


// 求非终结符的首符集
// 输入非终结符集合和CFG文法, 返回首符集
map<int, set<int>> getFirstSet(Grammar grammar);

// 求非终结符的随符集
// 输入非终结符集合和CFG文法, 返回随符集
map<int, set<int>> getFollowSet(Grammar grammar, map<int, set<int>> firstSet);

// lr(1)分析
// 输入非终结符集合和CFG文法, 返回nfa(lr(0)覆盖片映射和nfa邻接表)
pair<vector<LR1Cover>, EdgeTable> getLR1dfa(Grammar grammar);

void setLR1CoverExpanded(LR1Cover &cover, map<int, set<int>> followSet,
                         Grammar grammar);

// LR(1) analyze table
ActionTable getLR1table(Grammar grammar, vector<LR1Cover> covers,
                        EdgeTable edgeTable);

// lr(1)转lalr(1)
pair<vector<LR1Cover>, EdgeTable>
getLALR1fromLR1(Grammar grammar, vector<LR1Cover> covers, EdgeTable edgeTable);

} // namespace krill::grammar::core

#endif