#ifndef AUTOMATA_H
#define AUTOMATA_H
#include "defs.h"
#include <map>
#include <set>
#include <vector>
#include <string>
using std::pair;
using std::set, std::map, std::multimap, std::vector;

namespace krill::type {

using DFAgraph = map<int, map<int, int>>;      // {from, {symbol, to}}
using NFAgraph = map<int, multimap<int, int>>; // {from, {symbol, to}}

struct DFA {
    DFAgraph      graph;    // always assume start state = 0
    map<int, int> finality; // multiple final states can be binded differently
};

struct NFA {
    NFAgraph      graph;    // always assume start state = 0
    map<int, int> finality; // multiple final states can be binded differently
};

struct Edge {
    int symbol;
    int from;
    int to;
    // to make std::map happy
    bool operator<(const Edge &e) const;
    bool operator==(const Edge &e) const;
};
using EdgeTable = vector<Edge>;

std::string to_string(const EdgeTable &tbl, const map<int, std::string> &symbolNames);

} // namespace krill::type



namespace krill::automata {
    
using namespace krill::type;

DFA getMinimizedDfa(DFA dfa);
DFA getDFAfromNFA(NFA nfa);
DFA getDFAintegrated(vector<DFA> dfas);

// you don't use these
// -------------------

NFAgraph  toNFAgraph(EdgeTable edgeTable);
DFAgraph  toDFAgraph(EdgeTable edgeTable);
EdgeTable toEdgeTable(DFAgraph dfa);

DFA getReachableDfa(DFA dfa);
DFA getMergedDfa(DFA dfa);

using Closure    = set<int>;
using ClosureMap = map<int, Closure>; // {symbol, closure}

void                       setClosureExpanded(Closure &closure, NFAgraph nfa);
ClosureMap                 getNextClosures(Closure closure, NFAgraph nfa);
pair<DFAgraph, ClosureMap> getClosureMapfromNFAgraph(NFAgraph nfaGraph);
map<int, int>              getFinalityFromClosureMap(map<int, int>     nfaFinality,
                                                     map<int, Closure> closureMap);
DFA                        _getDFAintegrated(vector<DFA> dfas);
} // namespace krill::automata

#endif