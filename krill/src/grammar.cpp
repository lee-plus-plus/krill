#include "krill/grammar.h"
#include "fmt/format.h"
#include "krill/automata.h"
#include <queue>
#include <tuple>
using namespace krill::automata;

namespace krill::grammar {

vector<string> split(string str, const char *delim) {
    vector<string> res;
    char *         strc = &str[0];
    char *         temp = strtok(strc, delim);
    while (temp != NULL) {
        res.push_back(string(temp));
        temp = strtok(NULL, delim);
    }
    return res;
}

template <typename T1, typename T2> map<T2, T1> reverse(map<T1, T2> m) {
    map<T2, T1> m_reversed;
    for (auto[key, value] : m) { m_reversed[value] = key; }
    return m_reversed;
}

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

    symbolId["END_"] = END_SYMBOL;

    map<int, string> symbolNames = reverse(symbolId);
    return {prods, symbolNames};
}

bool Prod::operator<(const Prod &p) const {
    return std::tie(symbol, right) < std::tie(p.symbol, p.right);
}

bool Prod::operator==(const Prod &p) const {
    return std::tie(symbol, right) == std::tie(p.symbol, p.right);
}

Grammar::Grammar(set<int> terminalSet, set<int> nonterminalSet, vector<Prod> prods,
        map<int, string> symbolNames)
    : terminalSet(terminalSet), nonterminalSet(nonterminalSet), prods(prods),
      symbolNames(symbolNames) {}

Grammar::Grammar(vector<Prod> prods) : prods(prods) {
    for (const Prod &prod : prods) {
        nonterminalSet.insert(prod.symbol);
        for (int c : prod.right) { terminalSet.insert(c); }
    }
    for (int c : nonterminalSet) {
        if (terminalSet.count(c)) { terminalSet.erase(c); }
    }
    for (int c : terminalSet) { symbolNames[c] = fmt::format("_{:d}", c); }
    for (int c : nonterminalSet) { symbolNames[c] = fmt::format("_{:d}", c); }
}

Grammar::Grammar(vector<string> prodStrs) {
    auto[prods_, symbolNames_] = getProdsFromStr(prodStrs);
    prods                      = prods_;
    symbolNames                = symbolNames_;
    for (const Prod &prod : prods) {
        nonterminalSet.insert(prod.symbol);
        for (int c : prod.right) { terminalSet.insert(c); }
    }
    for (int c : nonterminalSet) {
        if (terminalSet.count(c)) { terminalSet.erase(c); }
    }
}

bool Token::operator<(const Token &t) const {
    return std::tie(id, lexValue) < std::tie(t.id, t.lexValue);
}

bool Token::operator==(const Token &t) const {
    return std::tie(id, lexValue) == std::tie(t.id, t.lexValue);
}

bool Token::operator!=(const Token &t) const {
    return std::tie(id, lexValue) != std::tie(t.id, t.lexValue);
}

// LR(1) action table
ActionTable getLR1table(Grammar grammar) {
    auto lr1Automata = core::getLR1Automata(grammar);
    auto lr1table    = core::getLR1table(grammar, lr1Automata);
    return lr1table;
}

// LALR(1) action table
ActionTable getLALR1table(Grammar grammar) {
    auto lr1Automata   = core::getLR1Automata(grammar);
    auto lalr1Automata = core::getLALR1fromLR1(grammar, lr1Automata);
    auto lalr1table    = core::getLR1table(grammar, lalr1Automata);
    return lalr1table;
}

} // namespace krill::grammar

namespace krill::grammar::core {

bool ProdItem::operator<(const ProdItem &p) const {
    return std::tie(symbol, right, dot) < std::tie(p.symbol, p.right, p.dot);
}

bool ProdItem::operator==(const ProdItem &p) const {
    return std::tie(symbol, right, dot) == std::tie(p.symbol, p.right, p.dot);
}

bool ProdLR1Item::operator<(const ProdLR1Item &p) const {
    return std::tie(symbol, right, dot, search) <
           std::tie(p.symbol, p.right, p.dot, p.search);
}

bool ProdLR1Item::operator==(const ProdLR1Item &p) const {
    return std::tie(symbol, right, dot, search) ==
           std::tie(p.symbol, p.right, p.dot, p.search);
}

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

LR1Automata getLR1Automata(Grammar grammar) {
    auto firstSet  = getFirstSet(grammar);
    auto followSet = getFollowSet(grammar, firstSet);
    // generate states
    vector<LR1State> states;
    LR1State         initStates = {
        {grammar.prods[0].symbol, grammar.prods[0].right, 0, END_SYMBOL}};
    setLR1StateExpanded(initStates, firstSet, grammar);
    states.push_back(initStates); // generate inital state

    // bfs, generate follow states
    EdgeTable edgeTable;
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
            setLR1StateExpanded(nextStates, firstSet, grammar);
            int tgtIdx;
            for (tgtIdx = 0; tgtIdx < states.size(); tgtIdx++) {
                if (nextStates == states[tgtIdx]) { break; }
            }
            if (tgtIdx == states.size()) { states.push_back(nextStates); }
            // printf("[%d,%d,%d]", symbol, i, tgtIdx);
            edgeTable.push_back({symbol, i, tgtIdx});
        }
    }

    return LR1Automata({states, edgeTable});
}

// expand the LR(1) state (epsilon-closure method)
void setLR1StateExpanded(LR1State &state, map<int, set<int>> firstSet,
                         Grammar grammar) {
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

        // (A -> α·B..., s), check prods with right started by B
        if (prodItem.dot + 1 < prodItem.right.size()) {
            // (A -> α·Bβ, s) => state += (B -> ·..., first(β))
            for (Prod prod : grammar.prods) {
                if (prod.symbol != prodItem.right[prodItem.dot]) { continue; }

                set<int> nextSet = firstSet[prodItem.right[prodItem.dot + 1]];
                for (int c : nextSet) {
                    ProdLR1Item nextProdItem({prod.symbol, prod.right, 0, c});
                    if (state.count(nextProdItem) == 0) {
                        state.insert(nextProdItem);
                        q.push(nextProdItem);
                    }
                }
            }
        } else {
            // (A -> α·B, s) => state += (B -> ·..., s)
            for (Prod prod : grammar.prods) {
                if (prod.symbol != prodItem.right[prodItem.dot]) { continue; }
                ProdLR1Item nextProdItem(
                    {prod.symbol, prod.right, 0, prodItem.search});
                if (state.count(nextProdItem) == 0) {
                    state.insert(nextProdItem);
                    q.push(nextProdItem);
                }
            }
        }
    }
}

ActionTable getLR1table(Grammar grammar, LR1Automata lr1Automata) {
    vector<LR1State> &states    = lr1Automata.states;
    EdgeTable &       edgeTable = lr1Automata.edgeTable;

    vector<Prod> &prods = grammar.prods;
    ActionTable   actionTable;

    // edges ==> ACTION and GOTO
    for (Edge edge : edgeTable) {
        if (grammar.terminalSet.count(edge.symbol)) {
            // s1 ——a-> s2 ==> action[s1, a] = s2
            actionTable[{edge.from, edge.symbol}] = {Action::ACTION, edge.to};
        } else {
            // s1 ——A-> s2 ==> goto[s1, A] = s2
            actionTable[{edge.from, edge.symbol}] = {Action::GOTO, edge.to};
        }
    }
    // node ==> REDUCE and ACCEPT
    for (int i = 0; i < states.size(); i++) {
        for (ProdLR1Item item : states[i]) {
            if (item.symbol == prods[0].symbol &&
                item.dot == item.right.size()) {
                // s1: (S -> ...·, #) ==> accept[s1, #]
                actionTable[{i, item.search}] = {Action::ACCEPT, 0};
            } else if (item.dot == item.right.size()) {
                // s1: (A -> ...·, s), r2: A -> ... ==> reduce[s1, s] = r2
                for (int j = 0; j < prods.size(); j++) {
                    if (prods[j].symbol == item.symbol &&
                        prods[j].right == item.right) {
                        actionTable[{i, item.search}] = {Action::REDUCE, j};
                        break;
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
    for (const auto[LALR1Idex, stateIdxs] : LALRIdx2stateIdxs) {
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