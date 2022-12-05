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
using krill::log::logger;
using std::min, std::max;
using std::stringstream;

namespace krill::type {

vector<Prod> getSymbolIdAssigned(const vector<vector<string>> &prodSymbolStrs,
                                 map<string, int> &            symbolIds) {
    vector<Prod> prods;
    int          symbolId =
        1 + apply_reduce(symbolIds, 258, [](int maxId, pair<string, int> elem) {
            return max(maxId, elem.second);
        });

    for (vector<string> words : prodSymbolStrs) {
        // assume words like {"Stmt", "Term", "'+'", "D-Term"}
        Prod currentProd;
        for (int i = 0; i < words.size(); i++) {
            const string &word = words[i];
            if (symbolIds.count(word) == 0) {
                if (word.size() == 3 && word[0] == '\'' && word[2] == '\'') {
                    // single char like "'+'"
                    symbolIds[word] = (int) word[1];
                } else if (word == "%prec") {
                    // drop all token after %prec (include %prec itself)
                    break;
                } else if (symbolIds.count(word) == 0) {
                    // not previous defined
                    logger.info("token {} not previous defined, added", word);
                    symbolIds[word] = symbolId++;
                }
            }
            if (i == 0) {
                currentProd.symbol = symbolIds[word];
            } else {
                currentProd.right.push_back(symbolIds[word]);
            }
        }
        prods.emplace_back(currentProd);
    }
    symbolIds["ζ"] = END_SYMBOL;
    symbolIds["ε"] = EMPTY_SYMBOL;
    spdlog::debug("symbolId after: {}", ToString{}(symbolIds));
    return prods;
}

// return NonterminalSet, TerminalSet
pair<set<int>, set<int>>
getTerminalAndNonterminalSet(const vector<Prod> &prods) {
    set<int> terminalSet;
    set<int> nonterminalSet;
    for (const Prod &prod : prods) {
        nonterminalSet.insert(prod.symbol);
        for (int c : prod.right) { terminalSet.insert(c); }
    }
    for (int c : nonterminalSet) {
        if (terminalSet.count(c)) { terminalSet.erase(c); }
    }
    return {terminalSet, nonterminalSet};
}

pair<vector<int>, vector<Associate>> getDefaultPriorityAndAsscociate(int size) {
    vector<int>       prodsPriority(size);
    vector<Associate> prodsAssociate(size);

    for (int i = 0; i < size; i++) {
        prodsPriority[i]  = i + 1;
        prodsAssociate[i] = Associate::kNone;
    }
    return {prodsPriority, prodsAssociate};
}

// generate prodsPriority, prodsAssociacity
pair<vector<int>, vector<Associate>>
getPriorityAndAsscociate(const vector<vector<string>> &prodSymbolStrs,
                         const map<string, int> &      symbolPriority,
                         const map<string, Associate> &symbolAssociate) {

    int size                    = prodSymbolStrs.size();
    auto[prodsPrior, prodsAsso] = getDefaultPriorityAndAsscociate(size);

    map<int, int>       priors;
    map<int, Associate> assos;
    for (int i = 0; i < prodSymbolStrs.size(); i++) {
        // assume words like {"Stmt", "Term", "'+'", "D-Term"}
        const vector<string> &words = prodSymbolStrs[i];
        for (const string &word : words) {
            if (word == "%prec") {
                continue; // drop
            }
            // latter overwrite fronter
            // TODO: if multiple definitions in same prod, log warnings
            if (symbolPriority.count(word) > 0) {
                int prior = symbolPriority.at(word);
                priors[i] = prior;
            }
            if (symbolAssociate.count(word) > 0) {
                auto asso = symbolAssociate.at(word);
                assos[i]  = asso;
            }
        }
    }

    for (auto[i, prior] : priors) { prodsPrior[i] = prior; }
    for (auto[i, asso] : assos) { prodsAsso[i] = asso; }
    return {prodsPrior, prodsAsso};
}

bool Prod::operator<(const Prod &p) const {
    return std::tie(symbol, right) < std::tie(p.symbol, p.right);
}

bool Prod::operator==(const Prod &p) const {
    return std::tie(symbol, right) == std::tie(p.symbol, p.right);
}

string Prod::str(const map<int, string> &symbolNames) const {
    stringstream ss;
    ss << fmt::format("{} -> {}", symbolNames.at(this->symbol),
                      fmt::join(apply_map(this->right, symbolNames), " "));
    return ss.str();
}

string to_string(const Associate &associate) {
    static string assoName[] = {"None", "Left", "Right"};
    return assoName[static_cast<int>(associate)];
}

Grammar::Grammar(set<int> ts, set<int> nts, vector<Prod> prods,
                 map<int, string> names, vector<int> priors,
                 vector<Associate> assos)
    : terminalSet(ts), nonterminalSet(nts), prods(prods), symbolNames(names),
      prodsPriority(priors), prodsAssociate(assos) {
    // fill default priority and asscociate
    if (priors.size() == 0 && assos.size() == 0) {
        auto[pirors_, assos_] = getDefaultPriorityAndAsscociate(prods.size());
        this->prodsPriority   = pirors_;
        this->prodsAssociate  = assos_;
    }
    // check
    assert(this->prods[0].right.size() == 1);
    assert(this->prodsPriority.size() == this->prods.size());
    assert(this->prodsAssociate.size() == this->prods.size());
}

Grammar::Grammar(vector<vector<string>> prodSymbolStrs,
                 map<string, int> symbolIds, map<string, int> symbolPriority,
                 map<string, Associate> symbolAssociate) {
    auto prods       = getSymbolIdAssigned(prodSymbolStrs, symbolIds);
    auto symbolNames = reverse(symbolIds);
    auto[ts, nts]    = getTerminalAndNonterminalSet(prods);
    auto[prodsPriors, prodsAssos] = getPriorityAndAsscociate(
        prodSymbolStrs, symbolPriority, symbolAssociate);
    new (this) Grammar(ts, nts, prods, symbolNames, prodsPriors, prodsAssos);
    logger.info("grammar generated:\n{}", this->str());
}

Grammar::Grammar(vector<string> prodStrs) {
    // assume prods like {"Term -> Term '+' D-Term"}
    // always drop 2nd elem
    vector<vector<string>> prodSymbolStrs;
    for (const string &prodStr : prodStrs) {
        prodSymbolStrs.push_back(split(prodStr, " "));
        prodSymbolStrs.back().erase(prodSymbolStrs.back().begin() + 1);
    }
    new (this) Grammar(prodSymbolStrs);
}

string Grammar::str() const {
    stringstream  ss;
    static string assoName[] = {"", "Left", "Righ"};
    ss << "Grammar: \n";
    ss << "Terminal set:\n  ";
    for (int s : this->terminalSet) {
        ss << fmt::format("{} ", this->symbolNames.at(s));
    }
    ss << "\nNon-terminal set:\n  ";
    for (int s : this->nonterminalSet) {
        ss << fmt::format("{} ", this->symbolNames.at(s));
    }
    ss << "\nProductions:\n   prior   idx  prod\n";
    for (int i = 0; i < prods.size(); i++) {
        ss << fmt::format("{:>4s}{:-4d}{:>6s}  {}\n",
                          assoName[static_cast<int>(prodsAssociate[i])],
                          prodsPriority[i], fmt::format("({:d})", i + 1),
                          prods[i].str(symbolNames));
    }
    return ss.str();
}

string Action::str() const {
    static string typeName[] = {"ACTION", "REDUCE", "GOTO", "ACCEPT"};
    stringstream  ss;
    ss << fmt::format("{} ", typeName[static_cast<int>(type)]);
    if (type == Action::Type::kAction || type == Action::Type::kGoto) {
        ss << fmt::format("s{:<2d}", tgt);
    } else if (type == Action::Type::kReduce) {
        ss << fmt::format("r{:<2d}", tgt + 1);
    }
    return ss.str();
}

string to_string(const ActionTable &tbl, const map<int, string> &symbolNames) {
    stringstream ss;
    ss << fmt::format("Action Table (size={}):\n", tbl.size());
    for (auto[key, action] : tbl) {
        ss << fmt::format("    s{:<2d} --> {:s} --> {}\n", key.first,
                          symbolNames.at(key.second), action.str());
    }
    return ss.str();
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
    ss << " ●";
    for (int i = this->dot; i < this->right.size(); i++) {
        ss << fmt::format(" {}", symbolNames.at(this->right[i]));
    }
    ss << fmt::format("  ⎥⎥ {}", symbolNames.at(this->search), this->search);
    return ss.str();
}

string LR1Automata::str(const map<int, string> &symbolNames) const {
    stringstream ss;
    ss << fmt::format("LR1 Automata (num_states={}, num_edges={})\n",
                      this->states.size(), this->edgeTable.size());
    int i = 0;
    for (LR1State state : this->states) {
        ss << fmt::format("{}): \n{}", i++, to_string(state, symbolNames));
    }
    ss << "\n" << to_string(this->edgeTable, symbolNames);
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
    logger.info("begin generating LR1 Automata");
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

    LR1Automata lr1Automata({states, edgeTable});
    logger.info(
        "complete generating LR1 Automata (num_states={}, num_edges={})",
        states.size(), edgeTable.size());
    logger.debug("LR1 Automata: \n{}", lr1Automata.str(grammar.symbolNames));
    return lr1Automata;
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

string to_string(const LR1State &state, const map<int, string> &symbolNames) {
    stringstream ss;
    for (auto it = state.begin(); it != state.end();) {
        ss << "  " << it->str(symbolNames);
        for (auto it_prev = it++;
             it != state.end() &&
             (static_cast<ProdItem>(*it) == static_cast<ProdItem>(*it_prev));
             it++) {
            ss << " " << symbolNames.at(it->search);
        }
        ss << "\n";
    }
    return ss.str();
}

ActionTable getLR1table(Grammar grammar, LR1Automata lr1Automata) {
    logger.info("begin generating LR1 Action Table");
    vector<LR1State> &states    = lr1Automata.states;
    EdgeTable &       edgeTable = lr1Automata.edgeTable;

    const auto &prods       = grammar.prods;
    const auto &symbolNames = grammar.symbolNames;
    ActionTable actionTable;

    // edges ==> ACTION and GOTO
    for (auto[search, from, to] : edgeTable) {
        bool isConflit = (actionTable.count({from, search}) != 0);
        if (isConflit) {
            // ACTION / ACTION confict
            Action theirAction = actionTable.at({from, search});
            Action ourAction   = {Action::Type::kAction, to};
            logger.critical(
                "lr1 conflit at state s{}, search {}: ours: {}, theirs: {}",
                symbolNames.at(search), ourAction.str(), theirAction.str());
            logger.info("summary of state s{}: \n{}", from,
                         to_string(states[from], symbolNames));
        }
        if (grammar.terminalSet.count(search)) {
            // s1 ——a-> s2 ==> action[s1, a] = s2
            actionTable[{from, search}] = {Action::Type::kAction, to};
        } else {
            // s1 ——A-> s2 ==> goto[s1, A] = s2
            actionTable[{from, search}] = {Action::Type::kGoto, to};
        }
        if (isConflit) {
            logger.critical("conflit resolved by FORCING overwrite, may not "
                             "be what you want");
        }
    }
    // node ==> REDUCE and ACCEPT
    for (int i = 0; i < states.size(); i++) {
        for (ProdLR1Item item : states[i]) {
            // s1: (S -> ...·, #) ==> action[s1, #] = ACCEPT
            if (item.symbol == prods[0].symbol &&
                item.dot == item.right.size()) {
                actionTable[{i, item.search}] = {Action::Type::kAccept, 0};
                continue;
            } else if (item.dot != item.right.size()) {
                continue;
            }
            // s1: (A -> BC·, s), r2: A -> BC ==> action[s1, s] = REDUCE r2
            int r; // locate prod idx, denotes as r
            for (r = 0; r < prods.size(); r++) {
                if (prods[r].symbol == item.symbol &&
                    prods[r].right == item.right) {
                    break;
                }
            }

            if (actionTable.count({i, item.search}) == 0) {
                // no conflict, reduce
                actionTable[{i, item.search}] = {Action::Type::kReduce, r};
                continue;
            } else {
                // conflict
                Action theirAction = actionTable.at({i, item.search});
                Action ourAction   = {Action::Type::kReduce, r};
                logger.info(
                    "lr1 conflit at state s{}, search {}: ours: {}, theirs: {}",
                    i, symbolNames.at(item.search), ourAction.str(),
                    theirAction.str());

                // REDUCE / REDUCE conflict
                // (indicate that grammar is not lr(1) grammar)
                if (theirAction.type == Action::Type::kReduce) {
                    logger.critical("REDUCE / REDUCE conflict: state s{}, "
                                     "prod p{}, search {}:",
                                     i, r + 1, symbolNames.at(item.search));
                    int r2 = theirAction.tgt;
                    logger.critical("prod1 p{}: {}", r + 1,
                                     item.str(symbolNames));
                    logger.critical("prod2 p{}: {}", r2 + 1,
                                     prods[r2].str(symbolNames));
                    int p1 = grammar.prodsPriority[r];
                    int p2 = grammar.prodsPriority[r2];
                    logger.critical("conflict resolved: use p{} "
                                     "(priority p1={}, p2={})",
                                     ((p1 <= p2) ? (r + 1) : (r2 + 1)), p1, p2);
                    actionTable[{i, item.search}] =
                        (p1 <= p2) ? ourAction : theirAction;
                    continue;
                }

                // REDUCE / ACTION conflict
                assert(theirAction.type == Action::Type::kAction);
                logger.info("item p{}: {}", r + 1, item.str(symbolNames));

                int a;
                for (a = 0; a < prods.size(); a++) {
                    auto it = find(prods[a].right.begin(), prods[a].right.end(),
                                   item.search);
                    if (it != prods[a].right.end()) { break; }
                }
                int  rPrior = grammar.prodsPriority[r];
                int  aPrior = grammar.prodsPriority[a];
                auto asso   = grammar.prodsAssociate[r];

                if (rPrior != aPrior) {
                    // use PRIORITY to resolve conflict
                    bool useOurs = (rPrior < aPrior);
                    actionTable[{i, item.search}] =
                        useOurs ? ourAction : theirAction;
                    logger.info("conflict resolved: {} (priority r={}, a={})",
                                 (useOurs ? "REDUCE" : "ACTION"), rPrior,
                                 aPrior);
                    if (min(rPrior, aPrior) >= 0) {
                        // resolved by default priority, dangerous!
                        logger.critical("conflit resolved by default "
                                         "priority, may not be what you want");
                        logger.info("summary of state s{}: \n{}", i,
                                     to_string(states[i], symbolNames));
                    }
                } else if (asso != Associate::kNone) {
                    // use ASSOCIATIVITY to resolve conflict
                    bool useOurs = (asso == Associate::kLeft);
                    actionTable[{i, item.search}] =
                        useOurs ? ourAction : theirAction;
                    logger.info("lr1 conflict resolved: {} ({} associate)",
                                 (useOurs ? "REDUCE" : "ACTION"),
                                 (useOurs ? "Left" : "Right"));
                } else {
                    // no associativity (resolve failed), FORCING reduce
                    // dangerous!
                    actionTable[{i, item.search}] = ourAction;
                    logger.critical(
                        "failed to resolve by priority or associacity");
                    logger.critical("conflit resolved by FORCING reduce, may "
                                     "not be what you want");
                    logger.info("summary of state s{}: \n{}", i,
                                 to_string(states[i], symbolNames));
                }
            }
        }
    }

    logger.info("complete generating LR1 Action Table (size={})",
                 actionTable.size());
    logger.debug("LR1 ActionTable: \n{}", to_string(actionTable, grammar.symbolNames));
    return actionTable;
}

LR1Automata getLALR1fromLR1(Grammar grammar, LR1Automata lr1Automata) {
    logger.info("begin transferring LR1 Automata to LALR1 Automata");
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

    LR1Automata lalr1Automata({resStates, resEdgeTable});
    logger.info("complete transferring LR1 Automata to LALR1 Automata "
                 "(num_states={} -> {}, num_edges={} -> {})",
                 states.size(), resStates.size(), edgeTable.size(),
                 resEdgeTable.size());
    logger.debug("LALR1 Automata: \n{}", lalr1Automata.str(grammar.symbolNames));
    return lalr1Automata;
}

} // krill::grammar::core