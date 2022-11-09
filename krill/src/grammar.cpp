#include "krill/grammar.h"
#include "krill/automata.h"
#include <queue>
using namespace krill::automata;
using std::queue;
// using namespace krill::grammar;

namespace krill::grammar {

bool Prod::operator<(const Prod &p) const {
    return (symbol < p.symbol || (symbol == p.symbol && (right < p.right)));
}

bool Prod::operator==(const Prod &p) const {
    return (symbol == p.symbol && right == p.right);
}

bool ProdItem::operator<(const ProdItem &p) const {
    return (symbol < p.symbol ||
            (symbol == p.symbol &&
             (right < p.right || (right == p.right && (dot < p.dot)))));
}

bool ProdItem::operator==(const ProdItem &p) const {
    return (symbol == p.symbol && right == p.right && dot == p.dot);
}

Grammar::Grammar(vector<Prod> prods) : prods(prods) {
    for (const Prod &prod : prods) {
        nonterminalSet.insert(prod.symbol);
        for (int c : prod.right) { terminalSet.insert(c); }
    }
    for (int c : nonterminalSet) {
        if (terminalSet.count(c)) { terminalSet.erase(c); }
    }
}

// LR(1) analyze table
ActionTable getLR1table(Grammar grammar) {
    auto[covers, edgeTable] = core::getLR1dfa(grammar);
    auto lr1table           = core::getLR1table(grammar, covers, edgeTable);
    return lr1table;
}

// LALR(1) analyze table
ActionTable getLALR1table(Grammar grammar) {
    auto[covers0, edgeTable0] = core::getLR1dfa(grammar);
    auto[covers, edgeTable] =
        core::getLALR1fromLR1(grammar, covers0, edgeTable0);
    auto lalr1table = core::getLR1table(grammar, covers, edgeTable);
    return lalr1table;
}

} // namespace krill::grammar

namespace krill::grammar::core {
// ---

bool ProdLR1Item::operator<(const ProdLR1Item &p) const {
    return (symbol < p.symbol ||
            (symbol == p.symbol &&
             (right < p.right ||
              (right == p.right &&
               (dot < p.dot || (dot == p.dot && (search < p.search)))))));
}

bool ProdLR1Item::operator==(const ProdLR1Item &p) const {
    return (symbol == p.symbol && right == p.right && dot == p.dot &&
            search == p.search);
}

// 求首符集
map<int, set<int>> getFirstSet(Grammar grammar) {
    map<int, set<int>> firstSet;
    // first(a) = {a}
    for (int c : grammar.terminalSet) { firstSet[c] = {c}; }
    // first(A) = {}
    for (int symbol : grammar.nonterminalSet) { firstSet[symbol] = {}; }

    for (Prod prod : grammar.prods) {
        // prod: X -> ...
        if (prod.right.size() > 0 && grammar.terminalSet.count(prod.right[0])) {
            // X -> a... ==> first(X) += {a}
            firstSet[prod.symbol].insert(prod.right[0]);
        } else if (prod.right.size() == 0) {
            // X -> epsilon ==> first(X) += {epsilon}
            firstSet[prod.symbol].insert(EMPTY_SYMBOL);
        }
    }

    while (true) {
        bool isChanged = false;
        for (Prod prod : grammar.prods) {
            // prod: X -> ... (not empty)
            if (prod.right.size() == 0) { continue; }
            for (int i = 0; i < prod.right.size(); i++) {
                // prod: X -> Y0 Y1...Yi γ, epsilon ∈ first(Y0),...,first(Yi)
                // ==> first(X) += first(γ)
                int      y       = prod.right[i];
                set<int> tempSet = firstSet[y];
                tempSet.erase(EMPTY_SYMBOL);
                for (int elem : tempSet) {
                    if (firstSet[prod.symbol].count(elem) == 0) {
                        isChanged = true;
                        firstSet[prod.symbol].insert(elem);
                    }
                }

                if (firstSet[y].count(EMPTY_SYMBOL) == 0) {
                    break;
                } else {
                    // prod: X -> Y0 Y1...Yi, epsilon ∈ first(Y0),...,first(Yi)
                    // ==> first(X) += {epsilon}
                    if (i == prod.right.size() - 1) {
                        if (firstSet[prod.symbol].count(EMPTY_SYMBOL) == 0) {
                            isChanged = true;
                            firstSet[prod.symbol].insert(EMPTY_SYMBOL);
                        }
                    }
                }
            }
        }
        if (isChanged == false) { break; }
    }

    return firstSet;
}

// 求随符集
map<int, set<int>> getFollowSet(Grammar grammar, map<int, set<int>> firstSet) {
    map<int, set<int>> followSet;
    followSet[grammar.prods[0].symbol].insert(END_SYMBOL);

    while (true) {
        bool isChanged = false;
        for (Prod prod : grammar.prods) {
            // prod: B -> ...
            for (int i = 0; i < prod.right.size(); i++) {
                if (grammar.nonterminalSet.count(prod.right[i])) {
                    if (i != prod.right.size() - 1) {
                        // B -> αAβ ==> follow(A) += {first(β) - epsilon}
                        int      A       = prod.right[i];
                        int      b       = prod.right[i + 1];
                        set<int> tempSet = firstSet[b];
                        tempSet.erase(EMPTY_SYMBOL);
                        for (int elem : tempSet) {
                            if (followSet[A].count(elem) == 0) {
                                isChanged = true;
                                followSet[A].insert(elem);
                            }
                        }
                    } else {
                        // B -> αA ==> follow(A) += {follow(B)}
                        int A = prod.right[i];
                        int B = prod.symbol;
                        for (int elem : followSet[B]) {
                            if (followSet[A].count(elem) == 0) {
                                isChanged = true;
                                followSet[A].insert(elem);
                            }
                        }
                    }
                }
            }
        }
        if (isChanged == false) { break; }
    }

    return followSet;
}

// lr(1)分析
// 输入文法, 返回DFA和DFA节点所表示的lr(1)产生式项目
pair<vector<LR1Cover>, EdgeTable> getLR1dfa(Grammar grammar) {
    auto firstSet  = getFirstSet(grammar);
    auto followSet = getFollowSet(grammar, firstSet);
    // generate covers
    vector<LR1Cover> covers;
    LR1Cover         initCover = {
        {grammar.prods[0].symbol, grammar.prods[0].right, 0, END_SYMBOL}};
    setLR1CoverExpanded(initCover, firstSet, grammar);
    covers.push_back(initCover); // generate inital cover

    // bfs, generate follow covers
    EdgeTable edgeTable;
    for (int i = 0; i < covers.size(); i++) {
        map<int, LR1Cover> nextCovers;
        for (ProdLR1Item prodItem : covers[i]) {
            // current cover: (A -> α·Bβ, s)
            // B => next cover: (A -> αB·β, s)
            if (prodItem.dot < prodItem.right.size()) {
                int c = prodItem.right[prodItem.dot];
                if (nextCovers.count(c) == 0) { nextCovers[c] = {}; }
                nextCovers[c].insert({prodItem.symbol, prodItem.right,
                                      prodItem.dot + 1, prodItem.search});
            }
        }

        for (auto[symbol, nextCover] : nextCovers) {
            setLR1CoverExpanded(nextCover, firstSet, grammar);
            int tgtIdx;
            for (tgtIdx = 0; tgtIdx < covers.size(); tgtIdx++) {
                if (nextCover == covers[tgtIdx]) { break; }
            }
            if (tgtIdx == covers.size()) { covers.push_back(nextCover); }
            // printf("[%d,%d,%d]", symbol, i, tgtIdx);
            edgeTable.push_back({symbol, i, tgtIdx});
        }
    }

    return make_pair(covers, edgeTable);
}

// 扩张LR(1)覆盖片选择（epsilon-闭包法）
void setLR1CoverExpanded(LR1Cover &cover, map<int, set<int>> firstSet,
                         Grammar grammar) {
    queue<ProdLR1Item> q;
    for (ProdLR1Item prodItem : cover) { q.push(prodItem); }
    while (q.size()) { // bfs
        ProdLR1Item prodItem = q.front();
        q.pop();

        // (A -> α·, s) or (A -> α·aβ, s), no ε to expand
        if (prodItem.dot == prodItem.right.size() ||
            grammar.terminalSet.count(prodItem.right[prodItem.dot])) {
            continue;
        }

        // (A -> α·B..., s), check prods with right started by B
        if (prodItem.dot + 1 < prodItem.right.size()) {
            // (A -> α·Bβ, s) => cover += (B -> ·..., first(β))
            for (Prod prod : grammar.prods) {
                if (prod.symbol != prodItem.right[prodItem.dot]) { continue; }

                set<int> nextSet = firstSet[prodItem.right[prodItem.dot + 1]];
                for (int c : nextSet) {
                    ProdLR1Item nextProdItem({prod.symbol, prod.right, 0, c});
                    if (cover.count(nextProdItem) == 0) {
                        cover.insert(nextProdItem);
                        q.push(nextProdItem);
                    }
                }
            }
        } else {
            // (A -> α·B, s) => cover += (B -> ·..., s)
            for (Prod prod : grammar.prods) {
                if (prod.symbol != prodItem.right[prodItem.dot]) { continue; }
                ProdLR1Item nextProdItem(
                    {prod.symbol, prod.right, 0, prodItem.search});
                if (cover.count(nextProdItem) == 0) {
                    cover.insert(nextProdItem);
                    q.push(nextProdItem);
                }
            }
        }
    }
}

// LR(1) analyze table
ActionTable getLR1table(Grammar grammar, vector<LR1Cover> covers,
                        EdgeTable edgeTable) {
    vector<Prod> &prods = grammar.prods;
    ActionTable   analyzeTable;

    // edges ==> ACTION and GOTO
    for (Edge edge : edgeTable) {
        if (grammar.terminalSet.count(edge.symbol)) {
            // s1 ——a-> s2 ==> action[s1, a] = s2
            analyzeTable[{edge.from, edge.symbol}] = {Action::ACTION, edge.to};
        } else {
            // s1 ——A-> s2 ==> goto[s1, A] = s2
            analyzeTable[{edge.from, edge.symbol}] = {Action::GOTO, edge.to};
        }
    }
    // node ==> REDUCE and ACCEPT
    for (int i = 0; i < covers.size(); i++) {
        for (ProdLR1Item item : covers[i]) {
            if (item.symbol == prods[0].symbol &&
                item.dot == item.right.size()) {
                // s1: (S -> ...·, #) ==> accept[s1, #]
                analyzeTable[{i, item.search}] = {Action::ACCEPT, 0};
            } else if (item.dot == item.right.size()) {
                // s1: (A -> ...·, s), r2: A -> ... ==> reduce[s1, s] = r2
                for (int j = 0; j < prods.size(); j++) {
                    if (prods[j].symbol == item.symbol &&
                        prods[j].right == item.right) {
                        analyzeTable[{i, item.search}] = {Action::REDUCE, j};
                        break;
                    }
                }
            }
        }
    }
    return analyzeTable;
}

pair<vector<LR1Cover>, EdgeTable>
getLALR1fromLR1(Grammar grammar, vector<LR1Cover> covers, EdgeTable edgeTable) {
    // 反向寻找每个覆盖片的对应原始推导式
    map<ProdItem, int> prodIndex;
    int                i = 0;
    for (auto p : grammar.prods) {
        for (int j = 0; j < p.right.size() + 1; j++, i++) {
            prodIndex[{p.symbol, p.right, j}] = i;
        }
    }

    typedef set<int> ID;          // concentric ID
    map<int, ID>     coverIdx2ID; // <index LR1 covers, concentric ID>
    for (int i = 0; i < covers.size(); i++) {
        for (auto p : covers[i]) {
            ProdItem prod = {p.symbol, p.right, p.dot};
            coverIdx2ID[i].insert(prodIndex[prod]);
        }
    }
    map<ID, int> ID2LALRIdx;
    int          numLALR1Covers = 0;
    for (auto[coverIdx, ID] : coverIdx2ID) {
        if (ID2LALRIdx.count(ID) == 0) {
            ID2LALRIdx[ID] = numLALR1Covers;
            numLALR1Covers++;
        }
    }
    map<int, int>      coverIdx2LALRIdx;
    map<int, set<int>> LALRIdx2coverIdxs;
    for (auto[coverIdx, ID] : coverIdx2ID) {
        int LALRIdx                = ID2LALRIdx[ID];
        coverIdx2LALRIdx[coverIdx] = LALRIdx;
        LALRIdx2coverIdxs[LALRIdx].insert(coverIdx);
    }

    // map
    vector<LR1Cover> resCovers;
    EdgeTable        resEdgeTable;
    set<Edge>        resEdgeTable0;
    for (const auto[LALR1Idex, coverIdxs] : LALRIdx2coverIdxs) {
        LR1Cover resCover;
        for (int idx : coverIdxs) {
            for (auto item : covers[idx]) { resCover.insert(item); }
        }
        resCovers.push_back(resCover);
    }
    for (const Edge &edge : edgeTable) {
        resEdgeTable0.insert({edge.symbol, coverIdx2LALRIdx[edge.from],
                              coverIdx2LALRIdx[edge.to]});
    }
    for (auto edge : resEdgeTable0) { resEdgeTable.push_back(edge); }

    return make_pair(resCovers, resEdgeTable);
}

} // krill::grammar::core