#include "fmt/format.h"
#include "krill/grammar.h"
#include "krill/minic.h"
#include "krill/utils.h"
#include <cstdlib>
#include <fmt/color.h>
#include <iostream>
#include <map>
#include <sstream>
#include <vector>
using namespace std;
using namespace krill::type;
using namespace krill::utils;
using namespace krill::runtime;
using namespace krill::minic;

struct BasicBlock {
    Lbl  lbl;
    Code code;
};

template <typename T> using EdgeSet = set<pair<T, T>>;

pair<vector<BasicBlock>, EdgeSet<int>> getBasicBlocks(const Code &code) {
    vector<BasicBlock> blocks;
    EdgeSet<int>       blockEdges;

    // get basic block graph node
    Lbl           currLbl;
    EdgeSet<Lbl>  lblEdges;
    map<Lbl, int> toBlockIdx;
    for (const auto &q : code) {
        if (q.op == Op::kLabel) {
            currLbl             = q.addr1;
            toBlockIdx[currLbl] = blocks.size();
            blocks.push_back(BasicBlock{.lbl = currLbl, .code = {}});
        }
        blocks.back().code.push_back(q);
        if (q.op == Op::kBranch) {
            lblEdges.insert({currLbl, q.addr1});
            lblEdges.insert({currLbl, q.addr2});
        } else if (q.op == Op::kGoto) {
            lblEdges.insert({currLbl, q.addr1});
        }
    }
    // get basic block graph edges
    for (const auto & [ fromLbl, toLbl ] : lblEdges) {
        blockEdges.insert({toBlockIdx.at(fromLbl), toBlockIdx.at(toLbl)});
    }
    return {blocks, blockEdges};
}

void getIrLiveness(const QuadTuple &q, vector<Var> &defs, vector<Var> &uses) {
    // clang-format off
    switch (q.op) {
        case Op::kNop:
            break;
        case Op::kAssign:
            defs.push_back(q.args.dest);
            break;
        case Op::kCopy:
            uses.push_back(q.args.src1);
            defs.push_back(q.args.dest); // better to put defs after uses
            break;
        case Op::kAdd: case Op::kMinus: case Op::kMult: case Op::kDiv:
        case Op::kMod: case Op::kAnd: case Op::kOr: case Op::kXor:
        case Op::kNor: case Op::kLShift: case Op::kRShift: case Op::kEq:
        case Op::kNeq: case Op::kLeq: case Op::kLt:
            uses.push_back(q.args.src1);
            uses.push_back(q.args.src2);
            defs.push_back(q.args.dest); // better to put defs after uses
            break;
        case Op::kAllocate: case Op::kGlobal:
            assert(false); // should not occur in basic block
            break;
        case Op::kLoad: case Op::kStore:
            uses.push_back(q.args.var_m);
            uses.push_back(q.args.addr_m);
            break;
        case Op::kParamPut:
            uses.push_back(q.args.var_r);
            break;
        case Op::kParamGet:
            defs.push_back(q.args.var_r);
            break;
        case Op::kCall:
            if (q.args.var_r != var_empty) { defs.push_back(q.args.var_r); }
            break;
        case Op::kLabel: case Op::kGoto:
            // pass
            break;
        case Op::kBranch:
            uses.push_back(q.args.var_j);
            break;
        case Op::kRet:
            if (q.args.argc == 0) {
                // pass
            } else {
                uses.push_back(q.args.var_r);
            }
            break;
        case Op::kFuncBegin: case Op::kFuncEnd:
            assert(false); // should not occur in basic block
            break;
        default:
            assert(false);
    }
    // clang-format on
};

// get Interference Graph Edges over basic blocks
// by liveness analysis
EdgeSet<Var> getGlobalInfEdges(const vector<BasicBlock> &blocks,
                               const EdgeSet<int> &      blockEdges) {
    // liveness analysis
    // update DEF, USE
    auto defVars = vector<set<Var>>(blocks.size());
    auto useVars = vector<set<Var>>(blocks.size());
    for (int i = 0; i < blocks.size(); i++) {
        vector<Var> defs;
        vector<Var> uses;
        for (const auto &q : blocks[i].code) {
            getIrLiveness(q, defs, uses);
        }
        // DEF[i] = DEF[i] - USE[i], correctness ensured by SSA
        useVars[i] = to_set(uses);
        defVars[i] = SetOprter{to_set(defs)} - to_set(uses);
    }

    // assert all variables have both def and use
    auto allDefVars = set<int>{};
    auto allUseVars = set<int>{};
    for (const auto &s : defVars) { Appender{allDefVars}.append(s); }
    for (const auto &s : useVars) { Appender{allUseVars}.append(s); }
    assert(allDefVars == allUseVars);
    const auto &allVars = allDefVars;

    // update IN, OUT
    auto inVars      = vector<set<int>>(blocks.size());
    auto outVars     = vector<set<int>>(blocks.size());
    int  inVarsSize  = 0;
    int  outVarsSize = 0;
    while (true) {
        bool flag = false;
        for (int i : bidxs) {
            outVars_ = outVars[i];
            for (int succ : outEdges[i]) {
                outVars[i] = SetOprter{outVars[i]} | inVars[succ];
            }
            if (outVars[i] != outVars_) {
                flag      = true; // changed
                inVars[i] = (SetOprter{outVars[i]} - defVars[i]) | useVars[i];
            }
        }
        if (!flag) { break; }
    }

    // live range analysis
    // global (basic-block-crossing) register assignment
    auto globalRegVars = set<Var>{};
    auto map<Var, int> varsLiveRangeCount;
    for (int i = 0; i < blocks.size(); i++) {
        for (Var var : inVars[i]) { varsLiveRangeCount[var]++; }
    }
    for (const auto & [ var, count ] : varsLiveRangeCount) {
        if (count > 1) { globalRegVars.push_back(var); }
    }

    // construct interference graph
    EdgeSet<Var> interferenceEdges;
    for (int i = 0; i < blocks.size(); i++) {
        for (Var var1 : inVars[i]) {
            if (globalRegVars.count(var1) == 0) { continue; }
            for (Var var2 : inVars[i]) {
                if (globalRegVars.count(var2) == 0) { continue; }
                if (int(var2) <= int(var1)) { continue; } // asymmetric
                interferenceEdges.insert({var1, var2});
            }
        }
    }

    return interferenceEdges;
}

// get Interference Graph Edges in basic block
// by liveness analysis
EdgeSet<Var> getLocalInfEdges(const BasicBlock &block,
                              const set<Var> &  globalRegVars) {
    // liveness analysis
    // update DEF, USE
    auto varsLifetime = map<Var, pair<int, int>>;
    vector<Var> defsOrUses;
    for (const auto &q : blocks[i].code) {
        getIrLiveness(q, defsOrUses, defsOrUses);
    }
}


map<Var, int> coloring(EdgeSet<Var> infEdges) {
    map<Var, int>         infDegrees;
    map<Var, vector<Var>> infNeighbors;
    for (const auto & [ u, v ] : infEdges) {
        infDegrees[u]++;
        infDegrees[v]++;
        infNeighbors[u].push_back(v);
        infNeighbors[v].push_back(u);
    }

    // assign color by Kempe's Simplification & Greedy Coloring
    auto kempe_simplify = [infDegrees, infNeighbors]() -> vector<Var> {
        auto findMinDegreeNode = [const & degrees]()->pair<Var, int> {
            auto f = [](pair<Var, int> p0, pair<Var, int> p) {
                if (p.second == 0) { return p0; }
                if (p0.second == 0) { return p; }
                return (p.second < p0.second) ? p : p0;
            };
            return apply_reduce(infDegrees, make_pair(var_empty, 0), f);
        };
        vector<Var> result;
        while (true) {
            auto[var, k] = findMinDegreeNode();
            if (k == 0) { break; }
            result.push_back(var);
            for (Var var_n : infNeighbors[var]) { infDegrees[var_n]--; }
        }
        assert(result.size() == globalRegVars.size());
        return result;
    };
    vector<Var>   kempeVarsSeq = kempe_simplify();
    map<Var, int> varColor;
    int           numColor = 0;
    for (Var var : reverse(kempeVarsSeq)) {
        set<int> colors_n;
        for (Var var_n : infNeighbors[var]) {
            colors_n.insert(varColor[var_n]);
        }
        int color = 1;
        while (color.count(color) != 0) { color++; }
        varColor[Var] = color;
        numColor      = max(numColor, color);
    }
    return varColor;
}

void fuck(const Code &code) {
    auto[blocks, blockEdges] = getBasicBlocks(code);
    auto globalInfEdges      = getGlobalInfEdges(blocks, blockEdges);
    auto globalVarColors     = coloring(infEdges);

    auto localInfEdges
}