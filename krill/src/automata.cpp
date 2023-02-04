#include "krill/automata.h"
#include <exception>
#include <algorithm>
#include <queue>
#include <tuple>
#include <sstream>
using std::max, std::min;
using std::stringstream, std::string;

namespace krill::type {

bool Edge::operator<(const Edge &e) const {
    return std::tie(symbol, from, to) < std::tie(e.symbol, e.from, e.to);
}

bool Edge::operator==(const Edge &e) const {
    return std::tie(symbol, from, to) == std::tie(e.symbol, e.from, e.to);
}

string to_string(const EdgeTable &tbl, const map<int, string> &symbolNames) {
    stringstream ss;
    ss << fmt::format("EdgeTable (size={})\n", tbl.size());
    for (const Edge &edge : tbl) {
        ss << fmt::format("    s{:<2d} --> {:s} --> s{:<2d}\n", edge.from,
                           symbolNames.at(edge.symbol), edge.to);
    }
    return ss.str();
}


} // namespace krill::type



namespace krill::automata {

int match(DFA dfa, string input) {
    try {
        int state = 0;
        for (char c : input) { state = dfa.graph.at(state).at(c); }
        return dfa.finality.at(state);
    } catch (std::exception &err) { return false; }
}

bool unmatch(DFA dfa, string input) {
    try {
        int state = 0;
        for (char c : input) {
            if (dfa.graph.count(state) == 0) { return true; }
            if (dfa.graph.at(state).count(c) == 0) { return true; }
            state = dfa.graph.at(state).at(c);
        }
        return dfa.finality.at(state) == 0;
    } catch (std::exception &err) { return true; }
    return false;
}


// 最小化DFA
// 包括了消除不可达状态、合并等价状态
DFA getMinimizedDfa(DFA dfa) {
    return getReachableDfa(getMergedDfa(dfa));
}

// 将NFA转为DFA
// 采用默认方式确定覆盖片(DFA节点)的可终结属性
DFA getDFAfromNFA(NFA nfa) {
    auto[dfaGraph, closureMap] = getClosureMapfromNFAgraph(nfa.graph);
    auto finality = getFinalityFromClosureMap(nfa.finality, closureMap);
    return DFA({dfaGraph, finality});
}

// 将若干个DFA整合为1个大DFA
// 返回的大DFA的finality指示从原先第几个DFA的退出
// 原先的DFA的finality的含义被抹去
DFA getDFAintegrated(vector<DFA> dfas) {
    for (int i = 0; i < dfas.size(); i++) {
        dfas[i] = getMinimizedDfa(dfas[i]);
        for (auto it = dfas[i].finality.begin(); it != dfas[i].finality.end();
             it++) {
            if (it->second != 0) { it->second = i + 1; }
        }
    }
    return getMinimizedDfa(_getDFAintegrated(dfas));
}


// EdgeTabel => NFAgraph
NFAgraph toNFAgraph(EdgeTable edgeTable) {
    NFAgraph nfa;
    for (const Edge &edge : edgeTable) {
        nfa[edge.from].insert({edge.symbol, edge.to});
    }
    return nfa;
}

// EdgeTabel => DFAgraph
DFAgraph toDFAgraph(EdgeTable edgeTable) {
    DFAgraph dfa;
    for (const Edge &edge : edgeTable) {
        dfa[edge.from][edge.symbol] = edge.to;
    }
    return dfa;
}

// Dfagraph => EdgeTable
EdgeTable toEdgeTable(DFAgraph dfa) {
    EdgeTable edgeTable;
    for (const auto &node : dfa) {
        for (const auto &edge : node.second) {
            edgeTable.push_back({edge.first, node.first, edge.second});
        }
    }
    return edgeTable;
}

// NFAgraph转DFAgraph, 并返回覆盖片(DFAgraph结点)的构造
// 得到的DFA每个状态都对应NFA的一个覆盖片，得到的DFA并非最小
// 由于覆盖片中各节点可能拥有不同可终结属性, 覆盖片(DFA节点)的可终结属性无法确定
// 返回DFA和覆盖片，其中覆盖片记录了DFA-NFA节点映射关系
// 不负责处理转换后的可终结属性
pair<DFAgraph, ClosureMap> getClosureMapfromNFAgraph(NFAgraph nfaGraph) {
    vector<set<int>> closures; // dfa节点对原nfa节点的映射
    DFAgraph         dfaGraph;

    // 构造初始覆盖片
    set<int> initClosure({0});
    setClosureExpanded(initClosure, nfaGraph);
    closures.push_back(initClosure);

    // bfs, 产生新覆盖片
    for (int closureIdx = 0; (size_t) closureIdx < closures.size();
         closureIdx++) {
        set<int> closure = closures[closureIdx];

        set<int> nextSymbols;
        for (int state : closure) {
            for (const auto &node : nfaGraph[state]) {
                nextSymbols.insert(node.first);
            }
        }
        // 对覆盖片步进，产生新覆盖片
        map<int, set<int>> nextClosures = getNextClosures(closure, nfaGraph);
        for (const auto &elem : nextClosures) {
            int      symbol      = elem.first;
            set<int> nextClosure = elem.second;

            if (symbol == EMPTY_SYMBOL) { continue; }
            // 为覆盖片分配下一个idx，加入
            auto it = find(closures.begin(), closures.end(), nextClosure);
            int  nextClosureIdx = (int) (it - closures.begin());
            if ((size_t) nextClosureIdx == closures.size()) {
                closures.push_back(nextClosure); // 无重复, 先生成新覆盖片
            }
            dfaGraph[closureIdx][symbol] = nextClosureIdx;
        }
    }

    // 格式转换, closures -> closureMap
    map<int, set<int>> closureMap;
    for (int closureIdx = 0; (size_t) closureIdx < closures.size();
         closureIdx++) {
        for (int nfaState : closures[closureIdx]) {
            closureMap[closureIdx].insert(nfaState);
        }
    }

    return make_pair(dfaGraph, closureMap);
}

// 默认确定finality方式: 假定finality无冲突，若有多个则取最小值
// (finality越小越优先) 将若干个DFA整合为1个大DFA
// 每个DFA均以0为起始状态，合并得到唯一起始状态
// 用不同值标注DFA的节点可终结属性, 可以防止合并
// (得到的DFA未最小化)
map<int, int> getFinalityFromClosureMap(map<int, int> nfaFinality,
                                        ClosureMap    closureMap) {
    map<int, int> finality;
    for (const auto &elem : closureMap) {
        for (int i : elem.second) {
            finality[elem.first] =
                min(finality[elem.first], nfaFinality[i]) == 0
                    ? max(finality[elem.first], nfaFinality[i])
                    : min(finality[elem.first], nfaFinality[i]);
        }
    }
    return finality;
}

// 消除DFA不可达状态, 并整理DFA状态数字至0 ~ numStates-1
DFA getReachableDfa(DFA dfa) {
    DFA           resDfa;
    vector<int>   idState({0});
    map<int, int> stateId({{0, 0}});
    // bfs
    for (int id = 0; id < idState.size(); id++) {
        for (const auto &elem : dfa.graph[idState[id]]) {
            int symbol    = elem.first;
            int nextState = elem.second;
            // assign new id
            if (stateId.count(nextState) == 0) {
                stateId[nextState] = idState.size();
                idState.push_back(nextState);
            }
            // replace
            resDfa.graph[id][symbol] = stateId[nextState];
        }
    }
    for (const auto &elem : stateId) {
        resDfa.finality[elem.second] = dfa.finality[elem.first];
    }

    return resDfa;
}

// 合并DFA等价状态
// 可终结性相同且跳转等效的节点将会被合并
// 用不同值标注DFA的节点可终结属性, 可以防止合并
DFA getMergedDfa(DFA dfa) {
    // 用不同的整数(id)染色, 区分集合划分
    // 按可终结属性值的不同进行初次划分
    map<int, int> stateColor = dfa.finality;
    // 获取充分染色后DFA节点, 作为区分特征
    // 对于状态i和状态j,若{stateColor[i], dfa}
    using DFAnode          = pair<int, map<int, int>>;
    auto getColoredDFAnode = [&stateColor](DFAnode node) -> DFAnode {
        node.first = stateColor[node.first];
        for (auto it = node.second.begin(); it != node.second.end(); it++) {
            it->second = stateColor[it->second];
        }
        return node;
    };

    int                    numSplitedStates = 1;
    map<DFAnode, set<int>> partition;
    while (true) {
        // 按后继状态的染色情况是否相同, 进行进一步划分
        partition = {};
        for (auto[state, _] : dfa.finality) {
            auto coloredNode = getColoredDFAnode({state, dfa.graph[state]});
            partition[coloredNode].insert(state);
        }
        // 仅当无更新才结束
        if (numSplitedStates == partition.size()) { break; }
        numSplitedStates = partition.size();
        // 根据划分重新染色
        int newStateColor = 0;
        for (auto it = partition.begin(); it != partition.end(); it++) {
            for (int state : it->second) { stateColor[state] = newStateColor; }
            newStateColor++;
        }
    }
    // 根据染色, 构建状态替换映射
    map<int, int> idState;
    for (const auto &elem : stateColor) {
        if (idState.count(elem.second) == 0) {
            idState[elem.second] = elem.first;
        } else {
            idState[elem.second] = min(idState[elem.second], elem.first);
        }
    }
    map<int, int> replaceMap;
    for (const auto &elem : stateColor) {
        replaceMap[elem.first] = idState[stateColor[elem.first]];
    }
    // 替换
    EdgeTable edgeTable = toEdgeTable(dfa.graph);
    for (auto it = edgeTable.begin(); it != edgeTable.end(); it++) {
        it->from = replaceMap[it->from];
        it->to   = replaceMap[it->to];
    }
    DFA resDfa;
    resDfa.graph = toDFAgraph(edgeTable);
    map<int, int> resFinality;
    for (auto it = dfa.finality.begin(); it != dfa.finality.end(); it++) {
        resDfa.finality[replaceMap[it->first]] = it->second;
        // printf("{%d, %d} ", replaceMap[it->first], it->second);
    }

    return resDfa;
}

// // 扩张覆盖片选择（epsilon-闭包法）
void setClosureExpanded(Closure &closure, NFAgraph nfaGraph) {
    // bfs扩大搜索
    std::queue<int> q;
    for (int state : closure) { q.push(state); }
    while (q.size()) {
        int current = q.front();
        q.pop();
        for (const auto &elem : nfaGraph[current]) {
            int symbol = elem.first;
            int next   = elem.second;
            if (symbol == EMPTY_SYMBOL && closure.count(next) == 0) {
                closure.insert(next);
                q.push(next);
            }
        }
    }
}

// 求后继覆盖片
ClosureMap getNextClosures(Closure closure, NFAgraph nfaGraph) {
    map<int, set<int>> nextClosures;
    for (int current : closure) {
        for (const auto &edge : nfaGraph[current]) {
            if (edge.first != EMPTY_SYMBOL) {
                nextClosures[edge.first].insert(edge.second);
            }
        }
    }
    for (auto it = nextClosures.begin(); it != nextClosures.end(); it++) {
        setClosureExpanded(it->second, nfaGraph);
    }
    return nextClosures;
}

// 将若干个DFA整合为1个大DFA (for smarter person)
// 保留原先的DFA的finality的含义
// 如果你不明白原理，请使用 getDFAintegrated
DFA _getDFAintegrated(vector<DFA> dfas) {
    vector<EdgeTable> edgeTables(dfas.size());
    for (int i = 0; i < dfas.size(); i++) {
        edgeTables[i] = toEdgeTable(dfas[i].graph);
    }

    EdgeTable nfaEdgeTable;
    map<int, int> nfaFinality;

    // 更新规则很简单，就是把每个状态的重编号，把边集求合
    // states = {0, dfa0.states, dfa1.states, ...}
    int numStateAdded = 1;
    for (int i = 0; i < dfas.size(); i++) {
        // 更新边集合
        nfaEdgeTable.push_back({EMPTY_SYMBOL, 0, numStateAdded});
        for (Edge &edge : edgeTables[i]) {
            nfaEdgeTable.push_back({edge.symbol, edge.from + numStateAdded,
                                    edge.to + numStateAdded});
        }
        // 更新终结状态集合
        for (int state = 0; state < dfas[i].finality.size(); state++) {
            nfaFinality[state + numStateAdded] = dfas[i].finality[state];
        }

        numStateAdded += dfas[i].finality.size();
    }

    NFA nfa = {toNFAgraph(nfaEdgeTable), nfaFinality};
    DFA dfa = getDFAfromNFA(nfa);
    return dfa;
}

} // namespace krill::automata