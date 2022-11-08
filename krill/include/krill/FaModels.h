#ifndef FAMODELS_H
#define FAMODELS_H
#include "defs.h"
#include <map>
#include <set>
#include <vector>
using std::pair;
using std::set, std::map, std::multimap, std::vector;

namespace krill::lexical {

const int ERROR_STATE = -1; // dfa跳转错误状态, 作为dfa邻接矩阵的占位值
const int EMPTY_SYMBOL = 0; // 空字符ε, 作为nfa空边

using DFAgraph = map<int, map<int, int>>;      // {from, {symbol, to}}
using NFAgraph = map<int, multimap<int, int>>; // {from, {symbol, to}}

class DFA {
  public:
    DFAgraph graph;
    map<int, int> finality;
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

DFA getMinimizedDfa(DFA dfa);
DFA getDFAfromNFA(NFA nfa);
DFA getDFAintegrated(vector<DFA> dfas);

} // namespace krill::lexical

namespace krill::lexical::utils {
    using namespace krill::lexical;

    NFAgraph toNFAgraph(EdgeTable edgeTable);
    DFAgraph toDFAgraph(EdgeTable edgeTable);
    EdgeTable toEdgeTable(DFAgraph dfa);

    DFA getReachableDfa(DFA dfa);
    DFA getMergedDfa(DFA dfa);

    void setCoverExpanded(set<int> &cover, NFAgraph nfa);
    map<int, set<int>> getNextCovers(set<int> cover, NFAgraph nfa);
    pair<DFAgraph, map<int, set<int>>> getCoverMapfromNFAgraph(NFAgraph nfaGraph);
    map<int, int> getFinalityFromCoverMap(map<int, int> nfaFinality, map<int, set<int>> coverMap);
    DFA _getDFAintegrated(vector<DFA> dfas);
} // namespace krill:lexical::utils

#endif