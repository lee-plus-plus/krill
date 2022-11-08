#ifndef FAMODELS_H
#define FAMODELS_H
#include "defs.h"
#include <map>
#include <set>
#include <vector>
using std::map, std::multimap, std::vector;

namespace krill::lexical {

const int ERROR_STATE = -1; // dfa跳转错误状态, 作为dfa邻接矩阵的占位值
const int EMPTY_SYMBOL = 0; // 空字符ε, 作为nfa空边

using DFAgraph = map<int, map<int, int>>;      // {from, {symbol, to}}
using NFAgraph = map<int, multimap<int, int>>; // {from, {symbol, to}}

class DFA {
public:
    // always assume start state = 0
    DFAgraph graph;
    // multiple final states can be binded with different action
    map<int, int> finality;
    void print();
};
class NFA {
public:
    // always assume start state = 0
    NFAgraph graph;
    // multiple final states can be binded with different action
    map<int, int> finality;
};

class Edge {
public:
    int symbol;
    int from;
    int to;
    // to make std::map happy
    bool operator<(const Edge &e) const;
    bool operator==(const Edge &e) const;
};
using EdgeTable = vector<Edge>;

} // namespace krill::lexical
#endif