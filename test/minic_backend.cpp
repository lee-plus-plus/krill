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
#include <queue>
using namespace std;
using namespace krill::type;
using namespace krill::utils;
using namespace krill::runtime;
using namespace krill::minic;
using namespace krill::minic::ir;

using krill::log::logger;

string lbl_name(const Lbl &lbl);
string var_name(const Var &var);
string to_string(const Code &code);

extern map<Var, VarInfo> varInfo;
extern vector<FuncDecl> globalFuncDecls;
extern vector<VarDecl>  globalVarDecls;
extern bool isOpExpr(Op op);

extern MemInfo memInfo;
extern map<Var, VarInfo> varInfo;
extern map<Lbl, FuncInfo> funcInfo;

struct BasicBlock {
    Lbl  lbl;
    Code code;
};

vector<string> mipsCode;

template <typename T> using EdgeSet = set<pair<T, T>>;


void genCode(string op, string src1, string src2, string src3) {
    stringstream ss;
    ss << fmt::format("\t{} \t{}, {}, {}", op, src1, src2, src3);
    mipsCode.push_back(ss.str());
}

void genLabel(string label) {
    mipsCode.push_back(label + ":");
}

pair<vector<BasicBlock>, EdgeSet<int>> getBasicBlocks(const Code &code) {
    vector<BasicBlock> blocks;
    EdgeSet<int>       blockEdges;

    // get basic block graph node
    Lbl           currLbl;
    EdgeSet<Lbl>  lblEdges;
    map<Lbl, int> toBlockIdx;
    for (int i = 0; i < code.size(); i++) {
        const auto &q = code[i];
        if (q.op == Op::kFuncBegin) {
            currLbl             = q.args.func;
            toBlockIdx[currLbl] = blocks.size();
            blocks.push_back(BasicBlock{.lbl = currLbl, .code = {}});
        } else if (q.op == Op::kLabel) {
            currLbl             = q.args.addr1;
            toBlockIdx[currLbl] = blocks.size();
            blocks.push_back(BasicBlock{.lbl = currLbl, .code = {}});
        }
        assert(blocks.size() > 0);
        blocks.back().code.push_back(q);
        if (q.op == Op::kBranch) {
            lblEdges.insert({currLbl, q.args.addr1});
            lblEdges.insert({currLbl, q.args.addr2});
        } else if (q.op == Op::kGoto) {
            lblEdges.insert({currLbl, q.args.addr1});
        } else {
            if (i + 1 != code.size() && code[i + 1].op == Op::kLabel) {
                lblEdges.insert({currLbl, code[i + 1].args.addr1});
            }
        }
    }
    // get basic block graph edges
    for (const auto & [ fromLbl, toLbl ] : lblEdges) {
        blockEdges.insert({toBlockIdx.at(fromLbl), toBlockIdx.at(toLbl)});
    }
    
    logger.debug("{:d} basic blocks", blocks.size());
    for (const auto &block : blocks) {
        logger.debug("block {}:\n{}\n", lbl_name(block.lbl), to_string(block.code));
    }
    for (const auto & [ fromLbl, toLbl ] : lblEdges) {
        logger.debug("block {} -> {}", lbl_name(fromLbl), lbl_name(toLbl));
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
        case Op::kAdd: case Op::kMinus: case Op::kMult: case Op::kDiv:
        case Op::kMod: case Op::kAnd: case Op::kOr: case Op::kXor:
        case Op::kNor: case Op::kLShift: case Op::kRShift: case Op::kEq:
        case Op::kNeq: case Op::kLeq: case Op::kLt:
            uses.push_back(q.args.src1);
            uses.push_back(q.args.src2);
            defs.push_back(q.args.dest); // better to put defs after uses
            break;
        case Op::kAllocate: case Op::kGlobal:
            defs.push_back(q.args.var_a);
            // assert(false); // should not occur in basic block
            // pass
            break;
        case Op::kLoad: 
            defs.push_back(q.args.var_m);
            uses.push_back(q.args.addr_m);
            break;
        case Op::kStore:
            uses.push_back(q.args.var_m);
            uses.push_back(q.args.addr_m);
            break;
        case Op::kParamPut: case Op::kRetPut:
            uses.push_back(q.args.var_r);
            break;
        case Op::kParamGet: case Op::kRetGet:
            defs.push_back(q.args.var_r);
            break;
        case Op::kCall:
            if (q.args.var_r != var_empty) { defs.push_back(q.args.var_r); }
            break;
        case Op::kRet:
            if (q.args.argc == 0) {
                // pass
            } else {
                uses.push_back(q.args.var_r);
            }
            break;
        case Op::kLabel: case Op::kGoto:
            // pass
            break;
        case Op::kBranch:
            uses.push_back(q.args.var_j);
            break;
        case Op::kFuncBegin: case Op::kFuncEnd:
            // assert(false); // should not occur in basic block
            // pass
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
    logger.debug("global interference graph construction begin");
    vector<vector<int>> inEdges(blocks.size());
    vector<vector<int>> outEdges(blocks.size());
    for (const auto & [ from, to ] : blockEdges) {
        inEdges[to].push_back(from);
        outEdges[from].push_back(to);
    }
    vector<int> bidxs;
    for (int i = 0; i < blocks.size(); i++) {bidxs.push_back(i);}

    // liveness analysis
    // update DEF, USE
    auto regAssignNeed = [](const set<Var> &vars) -> set<Var> {
        set<Var> result;
        for (auto var : vars) {
            const auto info = varInfo.at(var);
            if (!(info.constVal.has_value() || info.fpOffset.has_value() ||
                info.memOffset.has_value())) {
                result.insert(var);
            }
        }
        return result;
    };
    auto defVars = vector<set<Var>>(blocks.size());
    auto useVars = vector<set<Var>>(blocks.size());
    for (int i = 0; i < blocks.size(); i++) {
        vector<Var> defs;
        vector<Var> uses;
        for (const auto &q : blocks[i].code) {
            getIrLiveness(q, defs, uses);
        }
        // DEF[i] = DEF[i] - USE[i], correctness ensured by SSA
        // correct my ass
        // useVars[i] = to_set(uses);
        useVars[i] = SetOprter{to_set(uses)} - to_set(defs);
        defVars[i] = to_set(defs);

        useVars[i] = regAssignNeed(useVars[i]);
        defVars[i] = regAssignNeed(defVars[i]);

        logger.debug("  USE[{}] = [{}]", lbl_name(blocks[i].lbl),
                     fmt::join(apply_map(to_vector(useVars[i]), var_name), " "));
        logger.debug("  DEF[{}] = [{}]", lbl_name(blocks[i].lbl),
                     fmt::join(apply_map(to_vector(defVars[i]), var_name), " "));
    }

    // update IN, OUT
    auto inVars      = vector<set<Var>>(useVars);
    auto outVars     = vector<set<Var>>(blocks.size());
    for (int t = 0;; t++) {
        bool flag = false;
        for (int i : bidxs) {
            auto outVars_ = outVars[i];
            outVars[i] = {};
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
    logger.debug("over the basic blocks {}");
    for (int i : bidxs) {
        logger.debug(
            "  IN[{}] = [{}]", lbl_name(blocks[i].lbl),
            fmt::join(apply_map(to_vector(inVars[i]), var_name), " "));
        logger.debug(
            "  OUT[{}] = [{}]", lbl_name(blocks[i].lbl),
            fmt::join(apply_map(to_vector(outVars[i]), var_name), " "));
        assert(inVars[i] == regAssignNeed(inVars[i]));
        assert(outVars[i] == regAssignNeed(outVars[i]));
    }

    // live range analysis
    // global (basic-block-crossing) register assignment
    set<Var> globalRegVars;
    map<Var, int> varsLiveRangeCount;
    for (int i = 0; i < blocks.size(); i++) {
        for (Var var : inVars[i]) { varsLiveRangeCount[var]++; }
    }
    for (const auto & [ var, count ] : varsLiveRangeCount) {
        if (count > 1) { globalRegVars.insert(var); }
    }

    // construct interference graph
    EdgeSet<Var> infEdges;
    for (int i = 0; i < blocks.size(); i++) {
        for (Var var1 : inVars[i]) {
            if (globalRegVars.count(var1) == 0) { continue; }
            for (Var var2 : inVars[i]) {
                if (globalRegVars.count(var2) == 0) { continue; }
                if (int(var2) <= int(var1)) { continue; } // asymmetric
                infEdges.insert({var1, var2});
            }
        }
    }

    logger.debug("interference edges (num={})", infEdges.size());
    vector<string> infVarnames =
        apply_map(to_vector(infEdges), [](pair<Var, Var> p) -> string {
            return var_name(p.first) + "-" + var_name(p.second);
        });
    logger.debug("edges:  {}", fmt::join(infVarnames, " "));
    logger.debug("global interference graph construction complete");

    return infEdges;
}

// get Interference Graph Edges in basic block
// by liveness analysis
EdgeSet<Var> getLocalInfEdges(const BasicBlock &block,
                              const set<Var> &  globalRegVars) {
    logger.debug("local interference graph construction begin");
    // liveness analysis
    set<Var> localVars;
    map<Var, pair<int, int>> varsLifetime;
    for (int i = 0; i < block.code.size(); i++) {
        const auto &q = block.code[i];
        vector<Var> defsOrUses;
        getIrLiveness(q, defsOrUses, defsOrUses);
        for (Var var : defsOrUses) {
            if (globalRegVars.count(var) > 0) { continue; }
            if (varsLifetime.count(var) == 0) {
                localVars.insert(var);
                varsLifetime[var] = {i+1, i+1};
            } else {
                varsLifetime[var].second = i+1;
            }
        }
    }
    logger.debug("local variables(num={}): {}", localVars.size(),
                 fmt::join(apply_map(to_vector(localVars), var_name), " "));
    logger.debug("lifetime: \n  {}",
                 fmt::join(apply_map(to_vector(localVars),
                                     [varsLifetime](Var var) -> string {
                                         auto[st, ed] = varsLifetime.at(var);
                                         return var_name(var) + ":[" +
                                                to_string(st) + "," +
                                                to_string(ed) + "]";
                                     }),
                           ", "));

    // construct interference graph
    EdgeSet<Var> infEdges;
    for (const auto &[var1, lifetime1] : varsLifetime) {
        // if (globalRegVars.count(var1) > 0) { continue; }
        for (const auto &[var2, lifetime2] : varsLifetime) {
            if (globalRegVars.count(var2) > 0) { continue; }
            if (int(var2) <= int(var1)) { continue; } // asymmetric
            auto [st1, ed1] = lifetime1;
            auto [st2, ed2] = lifetime2;
            if (max(st1, st2) <= min(ed1, ed2)) {
                infEdges.insert({var1, var2});
            }
        }
    }

    logger.debug("interference edges:\n  {}",
                 fmt::join(apply_map(to_vector(infEdges),
                                     [](pair<Var, Var> p) -> string {
                                         return var_name(p.first) + "-" +
                                                var_name(p.second);
                                     }),
                           " "));
    logger.debug("local interference graph construction complete");

    return infEdges;
}

map<Var, int> coloring(EdgeSet<Var> infEdges) {
    logger.debug("coloring begin");
    map<Var, int>         infDegrees;
    map<Var, vector<Var>> infNeighbors;
    for (const auto & [ u, v ] : infEdges) {
        assert(int(u) < int(v));
        infDegrees[u]++;
        infDegrees[v]++;
        infNeighbors[u].push_back(v);
        infNeighbors[v].push_back(u);
    }

    // assign color by Kempe's Simplification & Greedy Coloring
    auto kempe_simplify = [infDegrees, infNeighbors]() -> vector<Var> {
        auto infDegrees_ = infDegrees;
        auto findMinDegreeNode = [&infDegrees_]()->pair<Var, int> {
            auto f = [](pair<Var, int> p0, pair<Var, int> p) {
                if (p.second == 0) { return p0; }
                if (p0.second == 0) { return p; }
                return (p.second < p0.second) ? p : p0;
            };
            return apply_reduce(infDegrees_, make_pair(var_empty, 0), f);
        };
        vector<Var> result;
        while (true) {
            auto[var, k] = findMinDegreeNode();
            assert(k >= 0);
            // logger.debug("  find min-degree node: {} (deg={})", var_name(var), k);
            if (k == 0) { break; }
            result.push_back(var);
            infDegrees_[var] = 0;
            for (Var var_n : infNeighbors.at(var)) {
                infDegrees_[var_n] = max(infDegrees_[var_n] - 1, 0);
            }
        }
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
        while (colors_n.count(color) != 0) { color++; }
        varColor[var] = color;
        numColor      = max(numColor, color);
    }

    vector<string> varColorStrs = apply_map(
        to_vector(varColor), [](pair<Var, int> p) -> string {
            return var_name(p.first) + "=" + to_string(p.second);
        }
    );
    logger.debug("  {}", fmt::join(varColorStrs, " "));
    logger.debug("coloring complete ({} colors)", numColor);

    return varColor;
}

void assignVarsWithRegister(const Code &code) {
    auto[blocks, blockEdges] = getBasicBlocks(code);

    EdgeSet<Var>  globalInfEdges  = getGlobalInfEdges(blocks, blockEdges);
    map<Var, int> globalVarColors = coloring(globalInfEdges);
    set<Var>      globalVars;
    for (const auto &[var, color] : globalVarColors) {
        globalVars.insert(var);
    }
    
    vector<EdgeSet<Var>>  localInfEdges;
    vector<map<Var, int>> localVarColors;
    for (const auto &block : blocks) {
        auto infEdges = getLocalInfEdges(block, globalVars);
        auto varColors = coloring(infEdges);
        localInfEdges.push_back(infEdges);
        localVarColors.push_back(varColors);
    }

    auto getNumColors = [](const map<Var, int> &varColors) -> int {
        int numColors = 0;
        for (const auto &[var, color] : varColors) {
            numColors = max(numColors, color);
        }   
        return numColors;
    };
    auto getVarColorStr = [](map<Var, int> varColor) -> string {
        vector<string> varColorStrs = apply_map(
            to_vector(varColor), [](pair<Var, int> p) -> string {
                return var_name(p.first) + ": " + to_string(p.second);});
        return krill::utils::to_string(fmt::format("{}", fmt::join(varColorStrs, "\n")));
    };

    logger.info("global variables colors(num={}):\n\t{}", 
        getNumColors(globalVarColors),
        getVarColorStr(globalVarColors));
    for (int i = 0; i < blocks.size(); i++) {
        logger.info("{} local variables colors(num={}):\n\t{}", 
            lbl_name(blocks[i].lbl), 
            getNumColors(localVarColors[i]), 
            getVarColorStr(localVarColors[i]));
    }

    int numGlobalVarColors = getNumColors(globalVarColors);
    map<Var, int> colorMapping;
    for (auto [var, color] : globalVarColors) {
        colorMapping[var] = color;
    }
    for (auto localVarColors_ : localVarColors) {
        for (auto [var, color] : localVarColors_) {
            colorMapping[var] = numGlobalVarColors + color;
        }
    }
}

void genFuncBegin() {
    auto info = funcInfo.at(funcDecl.funcLbl.lbl)
    int spOffset = info.spOffset;

    genLabel(funcDecl.funcLbl.lblname);

    // 8($fp) = param<1> (x) 
    // 4($fp) = param<2> (y)
    // 0($fp) = $ra
    // -4($fp) = $fp_last
    // $fp - 8 = $sp 
    genCode("add", "$sp", "$fp", spOffset);
}

extern void varsNamingTest() {
    // initVarInfo();
    for (const auto &funcDecl : globalFuncDecls) {
        logger.debug("in function {}:", lbl_name(funcDecl.funcLbl.lbl));
        const auto &code = funcDecl.code;
        assignVarsWithRegister(code);
    }
}