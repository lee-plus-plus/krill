#include "krill/grammar.h"
#include "fmt/format.h"
#include "krill/automata.h"
#include "krill/utils.h"
#include "spdlog/spdlog.h"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <queue>
#include <sstream>
#include <tuple>
using namespace krill::utils;
using std::min, std::max;
using std::stringstream;

namespace krill::type {

// {"Term -> Term '+' Term D-Term", ... } =>
//   (Prod) {{256 -> 256 43 256 257}, ...},
//   (symbolNames) {{256, "Term"}, {43, "'+'"}, ...}
// assign id for symbols, in the order of occurrence
pair<vector<Prod>, map<int, string>> getProdsFromStr(vector<string> prodStrs) {
    vector<Prod>     prods;
    map<string, int> symbolId;
    int              numSymbols = 0;

    for (string prodStr : prodStrs) {
        vector<string> words = split(prodStr, " ");
        // assume words like {"Term", "->", "Term", "'+'", "D-Term"}
        int         symbol;
        vector<int> right;
        for (int i = 0; i < words.size(); i++) {
            if (i == 1) { continue; } // throw away the 2nd element "->"
            const string &word = words[i];
            if (symbolId.count(word) == 0) { // single char like "'+'"
                if (word.size() == 3 && word[0] == '\'' && word[2] == '\'') {
                    symbolId[word] = (int) word[1];
                } else { // long symbol like "Term"
                    symbolId[word] = 256 + numSymbols;
                    numSymbols++;
                }
            }
            if (i == 0) {
                symbol = symbolId[word];
            } else {
                right.push_back(symbolId[word]);
            }
        }
        prods.push_back({symbol, right});
    }

    symbolId["ζ"] = END_SYMBOL;
    symbolId["ε"] = EMPTY_SYMBOL;

    map<int, string> symbolNames = reverse(symbolId);
    return {prods, symbolNames};
}

bool Prod::operator<(const Prod &p) const {
    return std::tie(symbol, right) < std::tie(p.symbol, p.right);
}

bool Prod::operator==(const Prod &p) const {
    return std::tie(symbol, right) == std::tie(p.symbol, p.right);
}

string Prod::str(const map<int, string> &symbolNames) const {
    stringstream ss;
    ss << symbolNames.at(this->symbol);
    ss << " -> ";
    for (int i = 0; i < this->right.size(); i++) {
        ss << symbolNames.at(this->right[i]) << " ";
    }
    return ss.str();
}

Grammar::Grammar(set<int> terminalSet, set<int> nonterminalSet,
                 vector<Prod> prods, map<int, string> symbolNames,
                 map<int, int>       prodsPriority,
                 map<int, Associate> symbolAssociate)
    : terminalSet(terminalSet), nonterminalSet(nonterminalSet), prods(prods),
      symbolNames(symbolNames), prodsPriority(prodsPriority),
      symbolAssociate(symbolAssociate) {
    // check
    assert(prods[0].right.size() == 1);
    for (int i = 0; i < prods.size(); i++) {
        assert(prodsPriority.count(i) > 0);
    }
}

Grammar::Grammar(set<int> terminalSet, set<int> nonterminalSet,
                 vector<Prod> prods, map<int, string> symbolNames)
    : terminalSet(terminalSet), nonterminalSet(nonterminalSet), prods(prods),
      symbolNames(symbolNames) {
    assert(prods[0].right.size() == 1);
    for (int i = 0; i < prods.size(); i++) { prodsPriority[i] = i; }
}


Grammar::Grammar(vector<Prod> prods) : prods(prods) {
    for (const Prod &prod : prods) {
        nonterminalSet.insert(prod.symbol);
        for (int c : prod.right) { terminalSet.insert(c); }
    }
    assert(prods[0].right.size() == 1);
    for (int c : nonterminalSet) {
        if (terminalSet.count(c)) { terminalSet.erase(c); }
    }
    for (int c : terminalSet) { symbolNames[c] = fmt::format("_{:d}", c); }
    for (int c : nonterminalSet) { symbolNames[c] = fmt::format("_{:d}", c); }
    for (int i = 0; i < prods.size(); i++) { prodsPriority[i] = i; }
}

Grammar::Grammar(vector<string> prodStrs) {
    auto[prods_, symbolNames_] = getProdsFromStr(prodStrs);
    assert(prods_[0].right.size() == 1);
    prods       = prods_;
    symbolNames = symbolNames_;
    for (const Prod &prod : prods) {
        nonterminalSet.insert(prod.symbol);
        for (int c : prod.right) { terminalSet.insert(c); }
    }
    for (int c : nonterminalSet) {
        if (terminalSet.count(c)) { terminalSet.erase(c); }
    }
    for (int i = 0; i < prods.size(); i++) { prodsPriority[i] = i; }
}

} // namespace krill::type



namespace krill::grammar {

// LR(1) action table
ActionTable getLR1table(Grammar grammar) {
    auto lr1Automata = getLR1Automata(grammar);
    auto lr1table    = getLR1table(grammar, lr1Automata);
    return lr1table;
}

// LALR(1) action table
ActionTable getLALR1table(Grammar grammar) {
    auto lr1Automata   = getLR1Automata(grammar);
    auto lalr1Automata = getLALR1fromLR1(grammar, lr1Automata);
    auto lalr1table    = getLR1table(grammar, lalr1Automata);
    return lalr1table;
}

bool ProdItem::operator<(const ProdItem &p) const {
    return std::tie(symbol, right, dot) < std::tie(p.symbol, p.right, p.dot);
}

bool ProdItem::operator==(const ProdItem &p) const {
    return std::tie(symbol, right, dot) == std::tie(p.symbol, p.right, p.dot);
}

string ProdItem::str(const map<int, string> &symbolNames) const {
    stringstream ss;
    ss << fmt::format("  {} ->", symbolNames.at(this->symbol));
    for (int i = 0; i < this->dot; i++) {
        ss << fmt::format(" {}", symbolNames.at(this->right[i]));
    }
    ss << " .";
    for (int i = this->dot; i < this->right.size(); i++) {
        ss << fmt::format(" {}", symbolNames.at(this->right[i]));
    }
    return ss.str();
}

bool ProdLR1Item::operator<(const ProdLR1Item &p) const {
    return std::tie(symbol, right, dot, search) <
           std::tie(p.symbol, p.right, p.dot, p.search);
}

bool ProdLR1Item::operator==(const ProdLR1Item &p) const {
    return std::tie(symbol, right, dot, search) ==
           std::tie(p.symbol, p.right, p.dot, p.search);
}

string ProdLR1Item::str(const map<int, string> &symbolNames) const {
    stringstream ss;
    ss << fmt::format("  {} ->", symbolNames.at(this->symbol));
    for (int i = 0; i < this->dot; i++) {
        ss << fmt::format(" {}", symbolNames.at(this->right[i]));
    }
    ss << " .";
    for (int i = this->dot; i < this->right.size(); i++) {
        ss << fmt::format(" {}", symbolNames.at(this->right[i]));
    }
    ss << fmt::format(" || {}", symbolNames.at(this->search), this->search);
    return ss.str();
}

map<int, set<int>> getFirstSets(Grammar grammar) {
    map<int, set<int>> firstSets;
    // first(a) = {a}
    for (int c : grammar.terminalSet) { firstSets[c] = {c}; }
    // first(A) = {}
    for (int symbol : grammar.nonterminalSet) { firstSets[symbol] = {}; }

    for (Prod prod : grammar.prods) {
        // prod: X -> ...
        if (prod.right.size() > 0 && grammar.terminalSet.count(prod.right[0])) {
            // X -> a... ==> first(X) += {a}
            firstSets[prod.symbol].insert(prod.right[0]);
        } else if (prod.right.size() == 0) {
            // X -> epsilon ==> first(X) += {epsilon}
            firstSets[prod.symbol].insert(EMPTY_SYMBOL);
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
                set<int> tempSet = firstSets[y];
                tempSet.erase(EMPTY_SYMBOL);
                for (int elem : tempSet) {
                    if (firstSets[prod.symbol].count(elem) == 0) {
                        isChanged = true;
                        firstSets[prod.symbol].insert(elem);
                    }
                }

                if (firstSets[y].count(EMPTY_SYMBOL) == 0) {
                    break;
                } else {
                    // prod: X -> Y0 Y1...Yi, epsilon ∈ first(Y0),...,first(Yi)
                    // ==> first(X) += {epsilon}
                    if (i == prod.right.size() - 1) {
                        if (firstSets[prod.symbol].count(EMPTY_SYMBOL) == 0) {
                            isChanged = true;
                            firstSets[prod.symbol].insert(EMPTY_SYMBOL);
                        }
                    }
                }
            }
        }
        if (isChanged == false) { break; }
    }

    return firstSets;
}

// TODO
// 添加文法左递归检查，左递归消除

map<int, set<int>> getFollowSets(Grammar            grammar,
                                 map<int, set<int>> firstSets) {
    // 要求文法无左递归
    map<int, set<int>> followSets;
    followSets[grammar.prods[0].symbol].insert(END_SYMBOL);

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
                        set<int> tempSet = firstSets[b];
                        tempSet.erase(EMPTY_SYMBOL);
                        for (int elem : tempSet) {
                            if (followSets[A].count(elem) == 0) {
                                isChanged = true;
                                followSets[A].insert(elem);
                            }
                        }
                        if (firstSets[b].count(0)) {
                            // B -> αAβ (β nullable) ==> follow(A) += follow(β)
                            for (int elem : followSets[b]) {
                                if (followSets[A].count(elem) == 0) {
                                    isChanged = true;
                                    followSets[A].insert(elem);
                                }
                            }
                        }
                    } else {
                        // B -> αA ==> follow(A) += {follow(B)}
                        int A = prod.right[i];
                        int B = prod.symbol;
                        for (int elem : followSets[B]) {
                            if (followSets[A].count(elem) == 0) {
                                isChanged = true;
                                followSets[A].insert(elem);
                            }
                        }
                    }
                }
            }
        }
        if (isChanged == false) { break; }
    }

    return followSets;
}

LR1Automata getLR1Automata(Grammar grammar) {
    auto firstSets  = getFirstSets(grammar);
    auto followSets = getFollowSets(grammar, firstSets);
    // generate states
    vector<LR1State> states;
    LR1State         initStates = {
        {grammar.prods[0].symbol, grammar.prods[0].right, 0, END_SYMBOL}};
    setLR1StateExpanded(initStates, firstSets, grammar);
    states.push_back(initStates); // generate inital state

    // bfs, generate follow states
    EdgeTable edgeTable;
    EdgeTable edgePriority;
    for (int i = 0; i < states.size(); i++) {
        map<int, LR1State> nextStatess;
        for (ProdLR1Item prodItem : states[i]) {
            // current state: (A -> α·Bβ, s)
            // B => next state: (A -> αB·β, s)
            if (prodItem.dot < prodItem.right.size()) {
                int c = prodItem.right[prodItem.dot];
                if (nextStatess.count(c) == 0) { nextStatess[c] = {}; }
                nextStatess[c].insert({prodItem.symbol, prodItem.right,
                                       prodItem.dot + 1, prodItem.search});
            }
        }

        for (auto[symbol, nextStates] : nextStatess) {
            setLR1StateExpanded(nextStates, firstSets, grammar);
            int tgtIdx;
            for (tgtIdx = 0; tgtIdx < states.size(); tgtIdx++) {
                if (nextStates == states[tgtIdx]) { break; }
            }
            if (tgtIdx == states.size()) { states.push_back(nextStates); }

            edgeTable.push_back({symbol, i, tgtIdx});
        }
    }

    return LR1Automata({states, edgeTable});
}

// expand the LR(1) state (epsilon-closure method)
void setLR1StateExpanded(LR1State &state, map<int, set<int>> firstSets,
                         Grammar grammar) {
    // given αβ...γ, return {c | c in first(αβ...γ) and c is terminals}
    auto getSeqFirstSetTerminals =
        [grammar, firstSets](vector<int> symbolSeq) -> set<int> {
        set<int> seqFirstSet;
        for (int s : symbolSeq) {
            if (s == END_SYMBOL) {
                seqFirstSet.insert(s);
                break;
            }
            for (int t : firstSets.at(s)) {
                if (grammar.terminalSet.count(t)) { seqFirstSet.insert(t); }
            }
            if (firstSets.at(s).count(0) == 0) { break; }
        }
        return seqFirstSet;
    };

    std::queue<ProdLR1Item> q;
    for (ProdLR1Item prodItem : state) { q.push(prodItem); }
    while (q.size()) { // bfs
        ProdLR1Item prodItem = q.front();
        q.pop();

        // (A -> α·, s) or (A -> α·aβ, s), no ε to expand
        if (prodItem.dot == prodItem.right.size() ||
            grammar.terminalSet.count(prodItem.right[prodItem.dot])) {
            continue;
        }

        // for item (A -> α·Bβ, s) in item-set I:
        //   for all production (B -> γ):
        //     state += (B -> ·γ, {c | c in first(βs) and c is terminals})
        for (Prod prod : grammar.prods) {
            if (prod.symbol != prodItem.right[prodItem.dot]) { continue; }

            vector<int> afterSeq(prodItem.right.begin() + prodItem.dot + 1,
                                 prodItem.right.end());
            afterSeq.push_back(prodItem.search);
            set<int> nextSet = getSeqFirstSetTerminals(afterSeq);
            for (int c : nextSet) {
                ProdLR1Item nextProdItem({prod.symbol, prod.right, 0, c});
                if (state.count(nextProdItem) == 0) {
                    state.insert(nextProdItem);
                    q.push(nextProdItem);
                }
            }
        }
    }
}

string toStr(const LR1State &state, map<int, string> symbolNames) {
    stringstream ss;
    for (const auto &item : state) {
        ss << "  " << item.str(symbolNames) << "\n";
    }
    return ss.str();
}

ActionTable getLR1table(Grammar grammar, LR1Automata lr1Automata) {
    vector<LR1State> &states    = lr1Automata.states;
    EdgeTable &       edgeTable = lr1Automata.edgeTable;

    const auto &prods       = grammar.prods;
    const auto &symbolNames = grammar.symbolNames;
    ActionTable actionTable;

    // edges ==> ACTION and GOTO
    for (Edge edge : edgeTable) {
        if (actionTable.count({edge.from, edge.symbol}) != 0) {
            // ACTION / ACTION confict
            spdlog::warn(
                "lr1 table ACTION / ACTION confilct: of state s{}, symbol {}",
                edge.from, edge.symbol);
        }
        if (grammar.terminalSet.count(edge.symbol)) {
            // s1 ——a-> s2 ==> action[s1, a] = s2
            actionTable[{edge.from, edge.symbol}] = {ACTION, edge.to};
        } else {
            // s1 ——A-> s2 ==> goto[s1, A] = s2
            actionTable[{edge.from, edge.symbol}] = {GOTO, edge.to};
        }
    }
    // node ==> REDUCE and ACCEPT
    for (int i = 0; i < states.size(); i++) {
        for (ProdLR1Item item : states[i]) {
            if (item.symbol == prods[0].symbol &&
                item.dot == item.right.size()) {
                // s1: (S -> ...·, #) ==> action[s1, #] = ACCEPT
                actionTable[{i, item.search}] = {ACCEPT, 0};

            } else if (item.dot == item.right.size()) {
                // s1: (A -> BC·, s), r2: A -> BC ==> action[s1, s] = reduce r2
                int r;
                for (r = 0; r < prods.size(); r++) {
                    if (prods[r].symbol == item.symbol &&
                        prods[r].right == item.right) {
                        break;
                    }
                }

                if (actionTable.count({i, item.search}) == 0) {
                    // no conflict, reduce
                    actionTable[{i, item.search}] = {REDUCE, r};
                } else {
                    Action theirAction = actionTable.at({i, item.search});
                    Action ourAction   = {REDUCE, r};

                    // REDUCE / REDUCE confilct
                    // (indicate that grammar is not lr(1) grammar)
                    if (theirAction.type == REDUCE) {
                        spdlog::critical(
                            "lr1 REDUCE / REDUCE confilct: state s{}, "
                            "prod p{}, search {}:",
                            i, r + 1, symbolNames.at(item.search));
                        int r2 = theirAction.tgt;
                        spdlog::critical("prod1 p{}: {}", r + 1,
                                         item.str(symbolNames));
                        spdlog::critical("prod2 p{}: {}", r2 + 1,
                                         prods[r2].str(symbolNames));
                        int p1 = grammar.prodsPriority.at(r);
                        int p2 = grammar.prodsPriority.at(r2);
                        spdlog::critical("lr1 confilct resolved: use p{} "
                                         "(priority p1={}, p2={})",
                                         ((p1 <= p2) ? (r + 1) : (r2 + 1)), p1,
                                         p2);
                        actionTable[{i, item.search}] =
                            (p1 <= p2) ? ourAction : theirAction;
                        continue;
                    }

                    // REDUCE / ACTION confilct
                    assert(theirAction.type == ACTION);
                    spdlog::info("lr1 REDUCE / ACTION confilct: state s{}, "
                                 "prod p{}, search {}:",
                                 i, r + 1, symbolNames.at(item.search));
                    // spdlog::info("state s{}: \n{}", i,
                    //              toStr(states[i], symbolNames));
                    spdlog::info("item p{}: {}", r + 1, item.str(symbolNames));

                    int a;
                    for (a = 0; a < prods.size(); a++) {
                        auto it = find(prods[a].right.begin(),
                                       prods[a].right.end(), item.search);
                        if (it != prods[a].right.end()) { break; }
                    }
                    int rPrior = grammar.prodsPriority.at(r);
                    int aPrior = grammar.prodsPriority.at(a);

                    if (rPrior != aPrior) {
                        // use priority to resolve conflict
                        bool useOurs = (rPrior < aPrior);
                        actionTable[{i, item.search}] =
                            useOurs ? ourAction : theirAction;
                        spdlog::info(
                            "lr1 confilct resolved: {} (priority r={}, a={})",
                            (useOurs ? "REDUCE" : "ACTION"), rPrior, aPrior);
                        if (min(rPrior, aPrior) >= 0) {
                            // resolved by default priority, which is not
                            // recommended
                            spdlog::warn("lr1 conflit resolved by default "
                                         "priority, may not be what you want");
                        }
                    } else {
                        // use associativity to resolve conflict
                        spdlog::info("symbolAssociate.size = {}",
                                     grammar.symbolAssociate.size());
                        bool hasAssociativity = false;
                        int  assoSymbol;
                        for (int symbol : prods[r].right) {
                            if (grammar.symbolAssociate.count(symbol) > 0) {
                                hasAssociativity = true;
                                assoSymbol       = symbol;
                                break;
                            }
                        }
                        if (hasAssociativity) {
                            auto asso = grammar.symbolAssociate.at(assoSymbol);
                            bool useOurs = (asso == Grammar::Associate::kLeft);
                            actionTable[{i, item.search}] =
                                useOurs ? ourAction : theirAction;
                            spdlog::info("lr1 confilct resolved: {} ("
                                         "{} is {} associate)",
                                         (useOurs ? "REDUCE" : "ACTION"),
                                         symbolNames.at(assoSymbol),
                                         (useOurs ? "Left" : "Right"));
                        } else {
                            // no associativity (resolve failed), FORCING reduce
                            actionTable[{i, item.search}] = ourAction;
                            spdlog::info("lr1 confilct resolved: REDUCE "
                                         "(default Left associate)");
                            spdlog::warn("lr1 conflit resolved by FORCING "
                                         "reduce, may not be what you want");
                        }
                    }
                }
            }
        }
    }
    return actionTable;
}

LR1Automata getLALR1fromLR1(Grammar grammar, LR1Automata lr1Automata) {
    vector<LR1State> &states    = lr1Automata.states;
    EdgeTable &       edgeTable = lr1Automata.edgeTable;

    // 反向寻找每个覆盖片的对应原始推导式
    map<ProdItem, int> prodIndex;
    int                i = 0;
    for (auto p : grammar.prods) {
        for (int j = 0; j < p.right.size() + 1; j++, i++) {
            prodIndex[{p.symbol, p.right, j}] = i;
        }
    }

    using ID = set<int>;      // concentric ID
    map<int, ID> stateIdx2ID; // <index LR1 states, concentric ID>
    for (int i = 0; i < states.size(); i++) {
        for (auto p : states[i]) {
            ProdItem prod = {p.symbol, p.right, p.dot};
            stateIdx2ID[i].insert(prodIndex[prod]);
        }
    }
    map<ID, int> ID2LALRIdx;
    int          numLALR1States = 0;
    for (auto[stateIdx, ID] : stateIdx2ID) {
        if (ID2LALRIdx.count(ID) == 0) {
            ID2LALRIdx[ID] = numLALR1States;
            numLALR1States++;
        }
    }
    map<int, int>      stateIdx2LALRIdx;
    map<int, set<int>> LALRIdx2stateIdxs;
    for (auto[stateIdx, ID] : stateIdx2ID) {
        int LALRIdx                = ID2LALRIdx[ID];
        stateIdx2LALRIdx[stateIdx] = LALRIdx;
        LALRIdx2stateIdxs[LALRIdx].insert(stateIdx);
    }

    // map
    vector<LR1State> resStates;
    EdgeTable        resEdgeTable;
    set<Edge>        resEdgeTable0;
    for (const auto & [ LALR1Idex, stateIdxs ] : LALRIdx2stateIdxs) {
        LR1State resState;
        for (int idx : stateIdxs) {
            for (auto item : states[idx]) { resState.insert(item); }
        }
        resStates.push_back(resState);
    }
    for (const Edge &edge : edgeTable) {
        resEdgeTable0.insert({edge.symbol, stateIdx2LALRIdx[edge.from],
                              stateIdx2LALRIdx[edge.to]});
    }
    for (auto edge : resEdgeTable0) { resEdgeTable.push_back(edge); }

    return LR1Automata({resStates, resEdgeTable});
}

} // krill::grammar::core