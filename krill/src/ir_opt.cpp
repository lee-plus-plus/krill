#include "krill/ir_opt.h"
#include "fmt/format.h"
#include "krill/utils.h"
#include <cstdlib>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <vector>
using namespace std;
using namespace krill::type;
using namespace krill::utils;
using namespace krill::ir;

using krill::log::logger;

namespace krill::ir {

// ---------- utils function ----------

string block_name(const BasicBlock &b) { return lbl_name(b.lbl); };

string lbl_edge_name(const pair<Lbl *, Lbl *> &p) {
    return krill::utils::to_string(
        fmt::format("<{}, {}>", lbl_name(p.first), lbl_name(p.second)));
};

// string var_name(Var *var) { return var->name; }

// string lbl_name(Lbl *lbl) { return lbl->name; }

// ---------- IrOptimizer ----------

// simply split blocks by label
BasicBlockGraph IrOptimizer::getBasicBlocks(const Code &funcCode) {
    vector<BasicBlock> blocks;
    EdgeSet<int>       blockEdges;

    // get basic block graph node
    Lbl *           currLbl;
    EdgeSet<Lbl *>  lblEdges;
    map<Lbl *, int> toBlockIdx;
    for (int i = 0; i < funcCode.size(); i++) {
        const auto &q = funcCode[i];
        if (i == 0 && q.op != Op::kLabel) {
            currLbl             = ir_.labels.assign({.name = "."});
            toBlockIdx[currLbl] = blocks.size();
            blocks.push_back(BasicBlock{.lbl = currLbl, .code = {}});
        }
        if (q.op == Op::kLabel) {
            currLbl             = q.args_j.addr1;
            toBlockIdx[currLbl] = blocks.size();
            blocks.push_back(BasicBlock{.lbl = currLbl, .code = {}});
        }
        assert(blocks.size() > 0);

        blocks.back().code.push_back(q);
        if (q.op == Op::kBranch) {
            lblEdges.insert({currLbl, q.args_j.addr1});
            lblEdges.insert({currLbl, q.args_j.addr2});
        } else if (q.op == Op::kGoto) {
            lblEdges.insert({currLbl, q.args_j.addr1});
        } else {
            if (i + 1 < funcCode.size() && funcCode[i + 1].op == Op::kLabel) {
                lblEdges.insert({currLbl, funcCode[i + 1].args_j.addr1});
            }
        }
    }
    // get basic block graph edges
    for (const auto & [ fromLbl, toLbl ] : lblEdges) {
        blockEdges.insert({toBlockIdx.at(fromLbl), toBlockIdx.at(toLbl)});
    }

    logger.debug("basic blocks(num={}): {}", blocks.size(),
                 fmt::join(apply_map(blocks, block_name), ", "));
    logger.debug(
        "basic block edges(num={}): {}", blockEdges.size(),
        fmt::join(apply_map(to_vector(lblEdges), lbl_edge_name), ", "));

    return BasicBlockGraph{.blocks = blocks, .edges = blockEdges};
}

// combine attached basic block
BasicBlockGraph
IrOptimizer::simplifyBasicBlocks(const BasicBlockGraph &blockGraph) {
    const auto &blocks     = blockGraph.blocks;
    const auto &blockEdges = blockGraph.edges;

    vector<set<int>> inEdges(blocks.size());
    vector<int>      isAttached(blocks.size());
    fill(isAttached.begin(), isAttached.end(), true);
    for (const auto & [ from, to ] : blockEdges) {
        inEdges[to].insert(from);
        if (from != to - 1) { isAttached[to] = false; }
    }

    vector<int> attachedIdx(blocks.size());
    for (int i = 0, aidx = 0; i < blocks.size(); i++) {
        if (!isAttached[i]) { aidx++; }
        if (i == 0) { aidx = 0; }
        attachedIdx[i] = aidx;
        if (i != 0 && attachedIdx[i]) {
            logger.debug("block {} is attached to block {}",
                         lbl_fullname(blocks[i].lbl),
                         lbl_fullname(blocks[i - 1].lbl));
        }
    }

    vector<BasicBlock> blocks2;
    for (int i = 0; i < blocks.size(); i++) {
        if (!isAttached[i] || i == 0) {
            blocks2.push_back(blocks[i]);
        } else {
            Appender{blocks2.back().code}.append(blocks[i].code);
        }
    }
    EdgeSet<int> blockEdges2;
    for (auto[from, to] : blockEdges) {
        if (!isAttached[to]) {
            blockEdges2.insert({attachedIdx[from], attachedIdx[to]});
        }
    }

    // debug
    logger.debug("Basic Blocks simplification complete");
    for (int i = 0; i < blocks.size(); i++) {
        logger.debug("  {} block {}", (attachedIdx[i] % 2) ? "[" : "<",
                     lbl_name(blocks[i].lbl));
    }

    return BasicBlockGraph{.blocks = blocks2, .edges = blockEdges2};
}

void IrOptimizer::livenessAnalysis(const QuadTuple &q, vector<Var *> &defs,
                                   vector<Var *> &uses) {
    // clang-format off
    switch (q.op) {
        case Op::kNop:
            break;
        case Op::kAssign:
            defs.push_back(q.args_i.var);
            break;
        case Op::kAdd: case Op::kSub: case Op::kMult: case Op::kDiv:
        case Op::kMod: case Op::kAnd: case Op::kOr: case Op::kXor:
        case Op::kNor: case Op::kLShift: case Op::kRShift: case Op::kEq:
        case Op::kNeq: case Op::kLeq: case Op::kLt:
            uses.push_back(q.args_e.src1);
            uses.push_back(q.args_e.src2);
            defs.push_back(q.args_e.dest); // better to put defs after uses
            break;
        case Op::kAlloca: case Op::kGlobal:
            defs.push_back(q.args_d.var);
            break;
        case Op::kLoad: 
            defs.push_back(q.args_m.var);
            uses.push_back(q.args_m.mem);
            break;
        case Op::kStore:
            uses.push_back(q.args_m.var);
            uses.push_back(q.args_m.mem);
            break;
        case Op::kRet:
            // pass
            break;
        case Op::kLabel: case Op::kGoto:
            // pass
            break;
        case Op::kBranch:
            uses.push_back(q.args_j.var);
            break;
        case Op::kCall:
            // pass
            break;
        case Op::kParamPut: case Op::kRetPut:
            uses.push_back(q.args_f.var);
            break;
        case Op::kRetGet:
            defs.push_back(q.args_f.var);
            break;
        case Op::kFuncBegin: case Op::kFuncEnd:
            // pass
            break;
        default:
            assert(false);
    }
    // clang-format on
};

// get Interference Graph Edges over basic blocks
// by liveness analysis
InfGraph IrOptimizer::getGlobalInfGraph(const BasicBlockGraph &blockGraph) {
    // logger.debug("global interference graph construction begin");
    auto &blocks     = blockGraph.blocks;
    auto &blockEdges = blockGraph.edges;
    // build edge table
    vector<vector<int>> inEdges(blocks.size());
    vector<vector<int>> outEdges(blocks.size());
    for (const auto & [ from, to ] : blockEdges) {
        inEdges[to].push_back(from);
        outEdges[from].push_back(to);
    }
    vector<int> bidxs;
    for (int i = 0; i < blocks.size(); i++) { bidxs.push_back(i); }

    // liveness analysis
    // update DEF, USE
    auto regAssignNeeded = [](const set<Var *> &vars) -> set<Var *> {
        set<Var *> result;
        for (auto var : vars) {
            const auto info = var->info;
            if (!(info.fpOffset.has_value() || info.memName.has_value()) &&
                var != var_zero) {
                result.insert(var);
            }
        }
        return result;
    };
    auto defVars = vector<set<Var *>>(blocks.size());
    auto useVars = vector<set<Var *>>(blocks.size());
    for (int i = 0; i < blocks.size(); i++) {
        vector<Var *> defs;
        vector<Var *> uses;
        for (const auto &q : blocks[i].code) {
            livenessAnalysis(q, defs, uses);
        }
        // DEF[i] = DEF[i] - USE[i], correctness ensured by SSA
        // correct my ass
        // useVars[i] = to_set(uses);
        useVars[i] = SetOprter{to_set(uses)} - to_set(defs);
        defVars[i] = to_set(defs);

        useVars[i] = regAssignNeeded(useVars[i]);
        defVars[i] = regAssignNeeded(defVars[i]);

        // logger.debug(
        //     "  USE[{}] = [{}]", lbl_name(blocks[i].lbl),
        //     fmt::join(apply_map(to_vector(useVars[i]), var_name), " "));
        // logger.debug(
        //     "  DEF[{}] = [{}]", lbl_name(blocks[i].lbl),
        //     fmt::join(apply_map(to_vector(defVars[i]), var_name), " "));
    }

    // update IN, OUT
    auto inVars  = vector<set<Var *>>(useVars);
    auto outVars = vector<set<Var *>>(blocks.size());
    for (int t = 0;; t++) {
        bool flag = false;
        for (int i : bidxs) {
            auto outVars_ = outVars[i];
            outVars[i]    = {};
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
    // logger.debug("over the basic blocks");
    for (int i : bidxs) {
        // logger.debug("  IN[{}] = [{}]", lbl_name(blocks[i].lbl),
        //              fmt::join(apply_map(to_vector(inVars[i]), var_name), "
        //              "));
        // logger.debug(
        //     "  OUT[{}] = [{}]", lbl_name(blocks[i].lbl),
        //     fmt::join(apply_map(to_vector(outVars[i]), var_name), " "));
        assert(inVars[i] == regAssignNeeded(inVars[i]));
        assert(outVars[i] == regAssignNeeded(outVars[i]));
    }

    // live range analysis
    // global (basic-block-crossing) register assignment
    set<Var *>      globalRegVars;
    map<Var *, int> varsLiveRangeCount;
    for (int i = 0; i < blocks.size(); i++) {
        for (Var *var : inVars[i]) { varsLiveRangeCount[var]++; }
    }
    for (const auto & [ var, count ] : varsLiveRangeCount) {
        if (count > 1) { globalRegVars.insert(var); }
    }

    // construct interference graph
    EdgeSet<Var *> infEdges;
    for (int i = 0; i < blocks.size(); i++) {
        for (Var *var1 : inVars[i]) {
            if (globalRegVars.count(var1) == 0) { continue; }
            for (Var *var2 : inVars[i]) {
                if (globalRegVars.count(var2) == 0) { continue; }
                if (var2 <= var1) { continue; } // asymmetric
                infEdges.insert({var1, var2});
            }
        }
    }

    auto inf_edge_name = [](pair<Var *, Var *> p) {
        return fmt::format("<{}, {}>", var_name(p.first), var_name(p.second));
    };
    logger.debug(
        "  interference edges (num={}): {}", infEdges.size(),
        fmt::join(apply_map(to_vector(infEdges), inf_edge_name), ", "));
    // logger.debug("global interference graph construction complete");

    return InfGraph{.vars = globalRegVars, .edges = infEdges};
}

// get Interference Graph Edges in basic block
// by liveness analysis
InfGraph IrOptimizer::getLocalInfGraph(const BasicBlock &block,
                                       const set<Var *> &globalRegVars) {
    // logger.debug("local interference graph construction begin");
    // liveness analysis
    set<Var *>                 localVars;
    map<Var *, pair<int, int>> varsLifetime;
    for (int i = 0; i < block.code.size(); i++) {
        const auto &  q = block.code[i];
        vector<Var *> defsOrUses;
        livenessAnalysis(q, defsOrUses, defsOrUses);
        for (Var *var : defsOrUses) {
            // if (globalRegVars.count(var) > 0) { continue; }
            if (varsLifetime.count(var) == 0) {
                localVars.insert(var);
                varsLifetime[var] = {i + 1, i + 1};
            } else {
                varsLifetime[var].second = i + 1;
            }
        }
    }
    // remove vars that don't need to assign registers
    auto regAssignNeeded =
        [globalRegVars](const set<Var *> &vars) -> set<Var *> {
        set<Var *> result;
        for (auto var : vars) {
            if (globalRegVars.count(var) > 0) { continue; }
            const auto info = var->info;
            if (info.fpOffset.has_value() || info.memName.has_value() ||
                (var == var_zero)) {
                continue;
            }
            result.insert(var);
        }
        return result;
    };
    localVars = regAssignNeeded(localVars);

    auto lifetime_name = [varsLifetime](Var *var) -> string {
        auto[st, ed] = varsLifetime.at(var);
        return fmt::format("{}: [{},{}]", var_name(var), st, ed);
    };
    logger.debug("  local variables(num={}): {}", localVars.size(),
                 fmt::join(apply_map(to_vector(localVars), var_name), " "));
    logger.debug(
        "  lifetime: {}",
        fmt::join(apply_map(to_vector(localVars), lifetime_name), ", "));

    // construct interference graph
    EdgeSet<Var *> infEdges;
    for (const auto &var1 : localVars) {
        const auto &lifetime1 = varsLifetime.at(var1);
        for (const auto &var2 : localVars) {
            const auto &lifetime2 = varsLifetime.at(var2);
            if (globalRegVars.count(var2) > 0) { continue; }
            if (var2 <= var1) {
                continue; // asymmetric
            }
            auto[st1, ed1] = lifetime1;
            auto[st2, ed2] = lifetime2;
            if (max(st1, st2) < min(ed1, ed2)) {
                // %7 = %8 + %9
                // %7, %8 can be assigned same register
                infEdges.insert({var1, var2});
            }
        }
    }


    auto inf_edge_name = [](pair<Var *, Var *> p) {
        return fmt::format("<{} , {}>", var_name(p.first), var_name(p.second));
    };
    logger.debug(
        "  interference edges (num={}): {}", infEdges.size(),
        fmt::join(apply_map(to_vector(infEdges), inf_edge_name), ", "));

    return InfGraph{.vars = localVars, .edges = infEdges};
}

map<Var *, int> IrOptimizer::coloring(const InfGraph &infGraph) {
    // logger.debug("coloring begin");
    const auto &vars     = infGraph.vars;
    const auto &infEdges = infGraph.edges;
    // assign color by Kempe's Simplification & Greedy Coloring
    auto kempe_simplify = [](EdgeSet<Var *> edges) -> vector<Var *> {
        set<Var *>                remains;
        map<Var *, int>           degrees;
        map<Var *, vector<Var *>> neighbors;
        for (const auto & [ u, v ] : edges) {
            assert(u < v); // asymmetric
            remains.insert(u);
            remains.insert(v);
            degrees[u]++;
            degrees[v]++;
            neighbors[u].push_back(v);
            neighbors[v].push_back(u);
        }

        auto findMinDegreeVar = [degrees](set<Var *> remains) -> Var * {
            assert(remains.size() > 0);
            Var *minVar = *remains.begin();
            for (auto var : remains) {
                if (degrees.at(var) < degrees.at(minVar)) { minVar = var; }
            }
            return minVar;
        };

        vector<Var *> result;
        while (remains.size() > 0) {
            Var *var = findMinDegreeVar(remains);
            result.push_back(var);
            remains.erase(var);
            for (Var *var_n : neighbors.at(var)) { degrees[var_n] -= 1; }
        }
        return result;
    };

    map<Var *, int> varColor;
    int             numColor = 0;
    // vars which have inference edges
    vector<Var *>             kempeVarsSeq = kempe_simplify(infEdges);
    map<Var *, vector<Var *>> infNeighbors;
    for (const auto & [ u, v ] : infEdges) {
        assert(u < v);
        infNeighbors[u].push_back(v);
        infNeighbors[v].push_back(u);
    }
    for (Var *var : reverse(kempeVarsSeq)) {
        set<int> colors_n;
        for (auto var_n : infNeighbors[var]) {
            if (varColor.count(var_n)) { colors_n.insert(varColor.at(var_n)); }
        }
        int color = 1;
        while (colors_n.count(color) != 0) { color++; }
        varColor[var] = color;
        numColor      = max(numColor, color);
        // logger.debug("kempe: {}: color {}", var_name(var), color);
    }
    // vars which have no inference edges
    for (const auto &var : vars) {
        if (varColor.count(var) == 0) {
            varColor[var] = 1;
            numColor      = max(numColor, 1);
        }
    }

    vector<string> varColorStrs =
        apply_map(to_vector(varColor), [](pair<Var *, int> p) -> string {
            return var_name(p.first) + "=" + std::to_string(p.second);
        });
    // logger.debug("  {}", fmt::join(varColorStrs, " "));
    // logger.debug("coloring complete ({} colors)", numColor);

    assert(varColor.size() == vars.size());
    return varColor;
}

IrOptimizer &IrOptimizer::assignRegs() {
    for (auto &func : ir_.globalFuncs) {
        if (!func->code.has_value()) {
            // not linked, assign no reg
            continue;
        }
        const auto &code = func->code.value();

        // check whether there is inside call
        bool has_inside_call = false;
        for (const auto &q : code) {
            if (q.op == Op::kCall) { has_inside_call = true; }
        }
        func->info.hasInsideCall = {has_inside_call};

        // build basic block graph
        logger.debug("in {}:", func_fullname(func));
        auto  blockGraph = simplifyBasicBlocks(getBasicBlocks(code));
        auto &blocks     = blockGraph.blocks;

        // build global Interference Graph (over basic blocks)
        // and assign colors for registers
        logger.debug("in {}: global", func_name(func));
        auto globalInfGraph  = getGlobalInfGraph(blockGraph);
        auto globalVars      = globalInfGraph.vars;
        auto globalVarColors = coloring(globalInfGraph);

        // build local Interference Graph (on one basic block)
        // and assign colors for registers
        auto localVarColors = vector<map<Var *, int>>{};
        auto localVars      = vector<set<Var *>>{};
        for (const auto &block : blocks) {
            logger.debug("in {}: {}\n{}", func_name(func),
                         lbl_fullname(block.lbl), to_string(block.code));
            auto infGraph  = getLocalInfGraph(block, globalVars);
            auto varColors = coloring(infGraph);
            localVars.push_back(infGraph.vars);
            localVarColors.push_back(varColors);
        }

        // merge local and global colors
        auto mergeVarColors = [blocks, localVarColors,
                               globalVarColors]() -> map<Var *, int> {
            auto getNumColors = [](const map<Var *, int> &varColors) -> int {
                int numColors = 0;
                for (const auto & [ var, color ] : varColors) {
                    numColors = max(numColors, color);
                }
                return numColors;
            };
            int  numGlobalVarColors = getNumColors(globalVarColors);
            auto varColors          = map<Var *, int>{};

            // logger.info("global variables colors(num={})",
            //             getNumColors(globalVarColors));
            for (auto[var, color] : globalVarColors) {
                varColors[var] = color;
                // logger.info("  {}: {}", var_name(var), color);
            }
            for (int i = 0; i < localVarColors.size(); i++) {
                // logger.info("{} local variables colors(num={})",
                //             lbl_name(blocks[i].lbl),
                //             getNumColors(localVarColors[i]));
                for (auto[var, color] : localVarColors[i]) {
                    varColors[var] = numGlobalVarColors + color;
                    // logger.info("  {}: {}", var_name(var),
                    //             numGlobalVarColors + color);
                }
            }
            return varColors;
        };
        auto varColors = mergeVarColors();

        // assign general-purpose registers for var
        // notice: $t0 is reserved for compiler
        // DO NOT assgin $t0 to any variable!
        const string regName[19] = {"$t0", "$t1", "$t2", "$t3", "$t4", "$t5",
                                    "$t6", "$t7", "$t8", "$t9", "$s0", "$s1",
                                    "$s2", "$s3", "$s4", "$s5", "$s6", "$s7"};
        set<string>  generalRegsSavedSet;
        for (auto[var, color] : varColors) {
            // 1-7: $t1 - $t7, 8-9: $t8 - $t9, 10- 17: $s0 - $s7
            // remain $t0 for immediate load use
            assert(1 <= color && color <= 17);
            // TODO: if color > 17, spilling
            string reg    = regName[color];
            var->info.reg = {reg};
            generalRegsSavedSet.insert(reg);
        }
        // record those used general-purpose registers
        auto generalRegsSaved = to_vector(generalRegsSavedSet);


        auto regsSaved = vector<string>{};
        // if function has call insede, $ra and $fp should be preserved
        if (has_inside_call) { Appender{regsSaved}.append({"$ra", "$fp"}); }
        // all the used general-purpose registers should be preserved
        Appender{regsSaved}.append(generalRegsSaved);
        // saved
        func->info.regsSaved = {regsSaved};

        // previously assigned stack space for parameters and local variabels
        // now move there offset to assign stack space for preserving registers
        //
        // preserve {"$ra", "$fp", "$t1", "$t2"} 4 registers
        //
        // | ...        ...                         | ...        ...
        // | +12($fp)   param<2>                    | +12($fp)   param<2>
        // |  +8($fp)   param<1>                    |  +8($fp)   param<1>
        // |  +4($fp)   param<0>                    |  +4($fp)   param<0>
        // |  ---------------------  <insert   =>   |  ---------------------
        // |  -0($fp)   retval<0>                   |  -0($fp)   reg_save<0>
        // |  -4($fp)   local_var<0>                |  -4($fp)   reg_save<1>
        // |  -8($fp)   local_var<1>                |  ...       ...
        // |                                        |  ---------------------
        // |                                        |  -16($fp)   retval<0>
        // |                                        |  -20($fp)   local_var<0>
        // |                                        |  -24($fp)   local_var<1>
        //
        // | $sp = $fp - 12                     =>  | $sp = $fp - 12 (- 16)

        int spOffsetForRegSaved = 4 * regsSaved.size();
        // move stack offset
        for (auto &var : func->returns) {
            assert(var->info.fpOffset.has_value());
            var->info.fpOffset.value() -= spOffsetForRegSaved;
        }
        for (auto &var : func->localVars) {
            assert(var->info.fpOffset.has_value());
            var->info.fpOffset.value() -= spOffsetForRegSaved;
        }
        assert(func->info.spOffset.has_value());
        func->info.spOffset.value() -= spOffsetForRegSaved;

        // log debug
        logger.debug("in {}", func_fullname(func));
        logger.debug("  preserved registers: {}", fmt::join(regsSaved, ", "));
        for (const auto &var : globalVars) {
            logger.debug("  {} = {}", var_name(var), var->info.reg.value());
        }
        for (int i = 0; i < blocks.size(); i++) {
            logger.debug("in {}: {}", func_fullname(func),
                         lbl_fullname(blocks[i].lbl));
            for (const auto &var : localVars[i]) {
                logger.debug("  {} = {}", var_name(var), var->info.reg.value());
            }
        }
    }
    var_zero->info.reg = {"$zero"};

    logger.info("registers assignment complete successfully");
    return *this;
}

// ---------- Information Annotation ----------

bool isOpExpr(Op op) {
    switch (op) {
    case Op::kAdd:
    case Op::kSub:
    case Op::kMult:
    case Op::kDiv:
    case Op::kMod:
    case Op::kAnd:
    case Op::kOr:
    case Op::kXor:
    case Op::kNor:
    case Op::kLShift:
    case Op::kRShift:
    case Op::kEq:
    case Op::kNeq:
    case Op::kLeq:
    case Op::kLt:
        return true;
    default:
        return false;
    }
}

int calcExpr(Op op, int val1, int val2) {
    switch (op) {
    case Op::kAdd:
        return val1 + val2;
    case Op::kSub:
        return val1 - val2;
    case Op::kMult:
        return val1 * val2;
    case Op::kDiv:
        return val1 / val2;
    case Op::kMod:
        return val1 % val2;
    case Op::kAnd:
        return val1 & val2;
    case Op::kOr:
        return val1 | val2;
    case Op::kXor:
        return val1 ^ val2;
    case Op::kNor:
        return ~(val1 | val2);
    case Op::kLShift:
        return val1 << val2;
    case Op::kRShift:
        return val1 >> val2;
    case Op::kEq:
        return val1 == val2;
    case Op::kNeq:
        return val1 != val2;
    case Op::kLeq:
        return val1 <= val2;
    case Op::kLt:
        return val1 < val2;
    default:
        assert(false);
        return 0;
    }
}

IrOptimizer &IrOptimizer::annotateInfo() {
    // for data segment
    int memOffset = 0;

    // initlaize zero variable
    var_zero->info = Var::Info{.constVal = {0}};

    // initialize global variable info
    for (const auto &var : ir_.globalVars) {
        var->info = Var::Info{
            .memName   = {var->name},
            .memOffset = {memOffset},
        };
        memOffset += var->type.size();
    }

    // initialize local variable info
    for (auto &func : ir_.globalFuncs) {
        /**
         * e.g.: int func(int x, int y[10], int z);
         * given:
         *     12($fp) = param<0> (int32 x)
         *      8($fp) = param<1> (int32* y)
         *      4($fp) = param<2> (int32 z)
         *        $sp  = $fp
         * do:
         *     -0($fp) <- local_var<0> (int32 retval)
         *  x  -4($fp) <- local_var<3> (int32[10] z) (bug)
         *    -40($fp) <- local_var<3> (int32[10] z)
         *    -44($fp) <- local_var<4> (int32 a)
         *        $sp  <- $fp - 48
         **/

        // assign local stack space for parameters
        // e.g. 12($fp) for param<0>, 8($fp) for param<1>, 4($fp) for param<0>
        int paramOffset = 4 * func->params.size();
        for (auto &var : func->params) {
            var->info = Var::Info{.fpOffset = {paramOffset}};
            paramOffset -= 4;
        }

        // reverse 0($fp) for $fp, -4($fp) for $sp, -8($fp) for $ra
        // -12($fp) for retval (if there is), -16($fp) ... for local variables
        int spOffset = +4;
        for (auto &var : func->returns) {
            spOffset -= var->type.size(); // allocate actual spcae for variables
            var->info = Var::Info{.fpOffset = {spOffset}};
        }
        for (auto &var : func->localVars) {
            spOffset -= var->type.size(); // allocate actual spcae for variables
            var->info = Var::Info{.fpOffset = {spOffset}};
        }
        spOffset -= 4;

        // set function info ($sp size)
        func->info = Func::Info{.spOffset = {spOffset}};

        // assign const value for temporal variables
        map<Var *, QuadTuple *> cvalCode;
        if (func->code.has_value()) {
            for (auto &q : func->code.value()) {
                // set var info
                switch (q.op) {
                case Op::kAssign:
                    q.args_i.var->info = Var::Info{.constVal = {q.args_i.cval}};
                    break;
                case Op::kLoad:
                    q.args_m.var->info = Var::Info{};
                    break;
                case Op::kRetGet:
                    q.args_f.var->info = Var::Info{};
                    break;
                default:
                    if (isOpExpr(q.op)) { q.args_e.dest->info = Var::Info{}; }
                    break;
                }
            }
        }
    }
    return *this;
}

// ---------- const value propagatation ----------

IrOptimizer &IrOptimizer::propagateConstValue() {
    for (auto &func : ir_.globalFuncs) {
        // assign const value for temporal variables
        map<Var *, QuadTuple *> cvalCode;
        if (func->code.has_value()) {
            for (auto &q : func->code.value()) {
                // set var info
                switch (q.op) {
                case Op::kAlloca:
                case Op::kGlobal:
                    // pass
                    break;
                case Op::kAssign:
                    // logger.debug("const value {}", var_name(q.args_i.var));
                    q.args_i.var->info = Var::Info{.constVal = {q.args_i.cval}};
                    // q = QuadTuple{.op = Op::kNop}; // delete
                    cvalCode[q.args_i.var] = &q;
                    break;
                case Op::kLoad:
                    q.args_m.var->info = Var::Info{};
                    break;
                case Op::kRetGet:
                    q.args_f.var->info = Var::Info{};
                    break;
                default:
                    if (isOpExpr(q.op)) { q.args_e.dest->info = Var::Info{}; }
                }

                // const value propagation
                if (isOpExpr(q.op)) {
                    // const value propagation
                    Var * var_src1 = q.args_e.src1;
                    Var * var_src2 = q.args_e.src2;
                    Var * var_dest = q.args_e.dest;
                    auto  src1     = var_src1->info;
                    auto  src2     = var_src2->info;
                    auto &dest     = var_dest->info;

                    if (src1.constVal.has_value() &&
                        src2.constVal.has_value()) {
                        // logger.debug("const value propagation: {}",
                        // to_string(q));

                        int cval1     = src1.constVal.value();
                        int cval2     = src2.constVal.value();
                        int result    = calcExpr(q.op, cval1, cval2);
                        dest.constVal = {result};
                        // overwrite
                        q = QuadTuple{
                            .op     = Op::kAssign,
                            .args_i = {.var = var_dest, .cval = result}};

                        cvalCode[var_dest]     = &q;
                        *cvalCode.at(var_src1) = QuadTuple{.op = Op::kNop};
                        *cvalCode.at(var_src2) = QuadTuple{.op = Op::kNop};
                    }
                }
            }
        }

        // delete Nop
        func->code.value() =
            apply_filter(func->code.value(),
                         [](QuadTuple q) -> bool { return q.op != Op::kNop; });
    }
    return *this;
}



// ---------- Common Sub-expression Eliminatation ----------

PtrPool<DagNode> IrOptimizer::buildDAG(Code &code) {
    PtrPool<DagNode>      nodes;
    map<Var *, DagNode *> varNodes;
    map<Var *, DagNode *> memNodes;
    map<Expr, DagNode *>  exprNodes;
    map<int, DagNode *>   cvalNodes;

    // build DAG
    for (auto &q : code) {
        logger.debug("{}", to_string(q));

        auto op = q.op;
        if (op == Op::kAssign) { // set
            // (var, cval)
            Var *var  = q.args_i.var;
            int  cval = q.args_i.cval;

            // bind const value to DAG node
            DagNode *node;
            if (cvalNodes.count(cval)) { // already has sub-expression
                node = cvalNodes.at(cval);

                logger.debug("  already have, drop");
                node->defined.push_back(&q);
            } else {
                node = nodes.assign(DagNode{.var = var, .defined = {&q}});
            }
            cvalNodes[cval] = node;
            varNodes[var]   = node;

            logger.debug("  node {}: (constval {})", var_name(node->var), cval);

        } else if (isOpExpr(op)) { // set, use
            // (dest, src1, src2)
            Var *    v_src1 = q.args_e.src1;
            Var *    v_src2 = q.args_e.src2;
            Var *    v_dest = q.args_e.dest;
            DagNode *src1   = varNodes.count(v_src1)
                                ? varNodes.at(v_src1)
                                : nodes.assign(DagNode{.var = v_src1});
            DagNode *src2 = varNodes.count(v_src2)
                                ? varNodes.at(v_src2)
                                : nodes.assign(DagNode{.var = v_src2});

            assert(varNodes.count(v_dest) == 0); // SSA
            // bind const value to DAG node (dest)
            DagNode *node;
            if (exprNodes.count({q.op, src1, src2})) {
                // already has sub-expression
                node = exprNodes.at({q.op, src1, src2});

                logger.debug("  already have, drop");
                node->defined.push_back(&q);
            } else {
                node = nodes.assign(DagNode{.op      = op,
                                            .src1    = src1,
                                            .src2    = src2,
                                            .var     = v_dest,
                                            .defined = {&q}});
            }
            exprNodes[{op, src1, src2}] = node;
            varNodes[v_dest]            = node;

            // replace used var
            if (q.args_e.src1 != node->src1->var) {
                logger.debug("  replace {} by {}", var_name(q.args_e.src1),
                             var_name(node->src1->var));
                q.args_e.src1 = node->src1->var;
            }
            if (q.args_e.src2 != node->src2->var) {
                logger.debug("  replace {} by {}", var_name(q.args_e.src2),
                             var_name(node->src2->var));
                q.args_e.src2 = node->src2->var;
            }

            logger.debug("  node {} ({}, node {} "
                         ", node {})",
                         var_name(node->var), enum_name(op),
                         var_name(node->src1->var), var_name(node->src2->var));

        } else if (op == Op::kLoad) { // set, (use mem)
            // (var, mem)
            Var *var = q.args_m.var;
            Var *mem = q.args_m.mem;

            assert(varNodes.count(var) == 0); // SSA
            // bind const value to DAG node (var)
            DagNode *node;
            if (mem->info.fpOffset.has_value() ||
                mem->info.memName.has_value()) {
                if (memNodes.count(mem)) {
                    // alreadly loaded in a variable
                    node = memNodes.at(mem);

                    logger.debug("  already have, drop");
                    node->defined.push_back(&q);
                } else {
                    node = nodes.assign(DagNode{.var = var, .defined = {&q}});
                }
                memNodes[mem] = node;
                node->assignUsed();
            } else {
                varNodes.at(mem)->assignUsed();
                node = nodes.assign(DagNode{.var = var, .defined = {&q}});
            }

            varNodes[var] = node;

            logger.debug("  node {} (load from {})", var_name(node->var),
                         var_name(mem));

        } else if (op == Op::kStore) { // use, (set mem)
            // (var, mem)
            Var *    var = q.args_m.var;
            Var *    mem = q.args_m.mem;
            DagNode *node;

            assert(varNodes.count(var) > 0);
            node = varNodes.at(var);

            // var used
            if (varNodes.count(mem) > 0) { varNodes.at(mem)->assignUsed(); }
            node->assignUsed();

            // bind const value to DAG node (mem)
            if (mem->info.fpOffset.has_value() ||
                mem->info.memName.has_value()) {
                memNodes[mem] = node;
            }

            // node->referenced.push_back(&q);

            logger.debug("  node {}: (store to {})", var_name(node->var),
                         var_name(mem));

            // replace used var
            if (q.args_m.var != node->var) {
                logger.debug("  replace {} by {}", var_name(q.args_m.var),
                             var_name(node->var));
                q.args_m.var = node->var;
            }

        } else if (op == Op::kParamPut || op == Op::kRetPut) { // ref
            // (var, idx)
            Var *var = q.args_f.var;
            // int      idx  = q.args_f.idx;
            DagNode *node = varNodes.count(var)
                                ? varNodes.at(var)
                                : nodes.assign(DagNode{.var = var});

            // var used
            node->assignUsed();
            // node->referenced.push_back(&q);

            // replace used var
            if (q.args_f.var != node->var) {
                logger.debug("  replace {} by {}", var_name(q.args_f.var),
                             var_name(node->var));
                q.args_f.var = node->var;
            }

        } else if (op == Op::kRetGet) { // set
            // (var, idx)
            // (var, idx)
            Var *var = q.args_f.var;
            // int  idx = q.args_f.idx;

            assert(varNodes.count(var) == 0); // SSA
            // bind const value to DAG node (var)
            DagNode *node = nodes.assign(DagNode{.var = var, .defined = {&q}});
            varNodes[var] = node;

        } else if (op == Op::kBranch) { // ref
            // (var, idx)
            Var *    var  = q.args_j.var;
            DagNode *node = varNodes.count(var)
                                ? varNodes.at(var)
                                : nodes.assign(DagNode{.var = var});

            // var used
            node->assignUsed();
            // node->referenced.push_back(&q);

            // replace used var
            if (q.args_j.var != node->var) {
                logger.debug("  replace {} by {}", var_name(q.args_j.var),
                             var_name(node->var));
                q.args_j.var = node->var;
            }

        } else {
            // pass
        }
    }

    logger.debug("DAG nodes:");
    for (auto &node : nodes.elements()) {
        logger.debug("  node {}: ({} {} {}) {}", var_name(node->var),
                     node->op != Op::kNop ? enum_name(node->op) : "",
                     node->src1 != nullptr ? var_name(node->src1->var) : "",
                     node->src2 != nullptr ? var_name(node->src2->var) : "",
                     node->used ? "used" : "unused");
        for (auto q : node->defined) {
            logger.debug("  - def: {}", to_string(*q));
        }
        // for (auto q : node->referenced) {
        //     logger.debug("  - use: {}", to_string(*q));
        // }
    }

    return nodes;
}

IrOptimizer &IrOptimizer::eliminateCommonSubExpr() {
    for (const auto &func : ir_.globalFuncs) {
        if (!func->code.has_value()) {
            // not linked, assign no reg
            continue;
        }
        auto &funcCode = func->code.value();

        // build basic block graph
        logger.debug("in {}:", func_fullname(func));
        auto  blockGraph = simplifyBasicBlocks(getBasicBlocks(funcCode));
        auto &blocks     = blockGraph.blocks;

        // build global Interference Graph (over basic blocks)
        // and assign colors for registers
        logger.debug("in {}: global", func_name(func));

        auto optimizedCode = vector<Code>(blocks.size());

        for (auto &block : blocks) {
            auto &code = block.code;

            // build DAG, modify operends
            PtrPool<DagNode> nodes = buildDAG(code);

            // remove useless code via DAG
            vector<QuadTuple *> uselessCode;
            for (const auto &node : nodes.elements()) {
                if (node->used) {
                    Appender{uselessCode}.append(slice(node->defined, 1));
                } else {
                    Appender{uselessCode}.append(slice(node->defined, 0));
                }
            }
            set<QuadTuple *> uselessCodeSet = to_set(uselessCode);
            for (auto &q : code) {
                if (uselessCodeSet.count(&q)) { q.op = Op::kNop; }
            }
            code = apply_filter(code,
                                [](QuadTuple q) { return q.op != Op::kNop; });
        }

        Code result;
        for (const auto &block : blocks) {
            Appender{result}.append(block.code);
        }
        funcCode = result;
    }

    return *this;
}

// ---------- update basic blocks to ir ----------



} // namespace krill::ir