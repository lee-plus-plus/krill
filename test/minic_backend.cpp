#include "fmt/format.h"
#include "krill/grammar.h"
#include "krill/minic.h"
#include "krill/utils.h"
#include <cstdlib>
#include <fmt/color.h>
#include <iostream>
#include <map>
#include <queue>
#include <sstream>
#include <vector>
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

extern string    to_string(const QuadTuple &q);
extern QuadTuple gen_allocate_code(const VarDecl &decl, const Op &op);
extern int       get_type_size(TypeSpec basetype, vector<int> shape);
extern FuncDecl &get_function_by_name(const string &funcname);

extern map<Var, VarDecl *>  varDecls;
extern map<Lbl, LblDecl *>  lblDecls;
extern map<Lbl, FuncDecl *> funcDecls;
extern vector<FuncDecl>     globalFuncDecls;
extern vector<VarDecl>      globalVarDecls;
extern bool                 isOpExpr(Op op);

extern map<Var, VarInfo>  varInfo;
extern map<Lbl, FuncInfo> funcInfo;

struct BasicBlock {
    Lbl  lbl;
    Code code;
};

map<Var, string> varToReg;

FuncInfo *     currentFuncInfo;
vector<string> mipsCode;

template <typename T> using EdgeSet = set<pair<T, T>>;

void genSection(string src) {
    stringstream ss;
    ss << fmt::format("{}", src);
    mipsCode.push_back(ss.str());
}

void genComment(string src) {
    stringstream ss;
    ss << fmt::format("\t# {} ", src);
    mipsCode.push_back(ss.str());
}

void genData(string src, int size) {
    // var: .space 20
    stringstream ss;
    ss << fmt::format("\t{}: .space {}", src, size);
    mipsCode.push_back(ss.str());
}

void genCode(string op) {
    stringstream ss;
    ss << fmt::format("\t{} ", op);
    mipsCode.push_back(ss.str());
}

void genCode(string op, string src1) {
    stringstream ss;
    ss << fmt::format("\t{} \t{} ", op, src1);
    mipsCode.push_back(ss.str());
}

void genCode(string op, string src1, string src2) {
    stringstream ss;
    ss << fmt::format("\t{} \t{}, {} ", op, src1, src2);
    mipsCode.push_back(ss.str());
}

void genCode(string op, string src1, string src2, string src3) {
    stringstream ss;
    ss << fmt::format("\t{} \t{}, {}, {} ", op, src1, src2, src3);
    mipsCode.push_back(ss.str());
}

void genCode(string op, string src1, string src2, int src3) {
    stringstream ss;
    if (op == "lw" || op == "sw") {
        ss << fmt::format("\t{} \t{}, {}({}) ", op, src1, src3, src2);
    } else {
        ss << fmt::format("\t{} \t{}, {}, {} ", op, src1, src2, src3);
    }
    mipsCode.push_back(ss.str());
}

void genOffsetCode(string op, string src1, string addr, string offset) {
    stringstream ss;
    ss << fmt::format("\t{} \t{}, {}({}) ", op, src1, offset, addr);
    mipsCode.push_back(ss.str());
}

void genLabel(string label) { mipsCode.push_back(label + ": "); }

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
        logger.debug("block {}:\n{}\n", lbl_name(block.lbl),
                     to_string(block.code));
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
        case Op::kParam: case Op::kRetGet:
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
pair<set<Var>, EdgeSet<Var>> getGlobalInfGraph(const vector<BasicBlock> &blocks,
                                               const EdgeSet<int> &blockEdges) {
    logger.debug("global interference graph construction begin");
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
    auto regAssignNeed = [](const set<Var> &vars) -> set<Var> {
        set<Var> result;
        for (auto var : vars) {
            const auto info = varInfo.at(var);
            if (!(info.fpOffset.has_value() || info.memName.has_value()) &&
                var != var_zero) {
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
        for (const auto &q : blocks[i].code) { getIrLiveness(q, defs, uses); }
        // DEF[i] = DEF[i] - USE[i], correctness ensured by SSA
        // correct my ass
        // useVars[i] = to_set(uses);
        useVars[i] = SetOprter{to_set(uses)} - to_set(defs);
        defVars[i] = to_set(defs);

        useVars[i] = regAssignNeed(useVars[i]);
        defVars[i] = regAssignNeed(defVars[i]);

        logger.debug(
            "  USE[{}] = [{}]", lbl_name(blocks[i].lbl),
            fmt::join(apply_map(to_vector(useVars[i]), var_name), " "));
        logger.debug(
            "  DEF[{}] = [{}]", lbl_name(blocks[i].lbl),
            fmt::join(apply_map(to_vector(defVars[i]), var_name), " "));
    }

    // update IN, OUT
    auto inVars  = vector<set<Var>>(useVars);
    auto outVars = vector<set<Var>>(blocks.size());
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
    logger.debug("over the basic blocks {}");
    for (int i : bidxs) {
        logger.debug("  IN[{}] = [{}]", lbl_name(blocks[i].lbl),
                     fmt::join(apply_map(to_vector(inVars[i]), var_name), " "));
        logger.debug(
            "  OUT[{}] = [{}]", lbl_name(blocks[i].lbl),
            fmt::join(apply_map(to_vector(outVars[i]), var_name), " "));
        assert(inVars[i] == regAssignNeed(inVars[i]));
        assert(outVars[i] == regAssignNeed(outVars[i]));
    }

    // live range analysis
    // global (basic-block-crossing) register assignment
    set<Var>      globalRegVars;
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

    return {globalRegVars, infEdges};
}

// get Interference Graph Edges in basic block
// by liveness analysis
pair<set<Var>, EdgeSet<Var>> getLocalInfGraph(const BasicBlock &block,
                                              const set<Var> &  globalRegVars) {
    logger.debug("local interference graph construction begin");
    // liveness analysis
    set<Var>                 localVars;
    map<Var, pair<int, int>> varsLifetime;
    for (int i = 0; i < block.code.size(); i++) {
        const auto &q = block.code[i];
        vector<Var> defsOrUses;
        getIrLiveness(q, defsOrUses, defsOrUses);
        for (Var var : defsOrUses) {
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
    auto regAssignNeed = [globalRegVars](const set<Var> &vars) -> set<Var> {
        set<Var> result;
        for (auto var : vars) {
            if (globalRegVars.count(var) > 0) { continue; }
            const auto info = varInfo.at(var);
            if (info.fpOffset.has_value() || info.memName.has_value() ||
                (var == var_zero)) {
                continue;
            }
            result.insert(var);
        }
        return result;
    };
    localVars = regAssignNeed(localVars);


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
    for (const auto &var1 : localVars) {
        const auto &lifetime1 = varsLifetime.at(var1);
        for (const auto &var2 : localVars) {
            const auto &lifetime2 = varsLifetime.at(var2);
            if (globalRegVars.count(var2) > 0) { continue; }
            if (int(var2) <= int(var1)) { continue; } // asymmetric
            auto[st1, ed1] = lifetime1;
            auto[st2, ed2] = lifetime2;
            if (max(st1, st2) < min(ed1, ed2)) {
                // %7 = %8 + %9
                // %7, %8 can be assigned same register
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

    return {localVars, infEdges};
}

map<Var, int> coloring(set<Var> vars, EdgeSet<Var> infEdges) {
    logger.debug("coloring begin");

    // assign color by Kempe's Simplification & Greedy Coloring
    auto kempe_simplify = [](EdgeSet<Var> edges) -> vector<Var> {
        set<Var>              remains;
        map<Var, int>         degrees;
        map<Var, vector<Var>> neighbors;
        for (const auto & [ u, v ] : edges) {
            assert(int(u) < int(v));
            remains.insert(u);
            remains.insert(v);
            degrees[u]++;
            degrees[v]++;
            neighbors[u].push_back(v);
            neighbors[v].push_back(u);
        }

        auto findMinDegreeVar = [degrees](set<Var> remains) -> Var {
            assert(remains.size() > 0);
            Var minVar = *remains.begin();
            for (auto var : remains) {
                if (degrees.at(var) < degrees.at(minVar)) { minVar = var; }
            }
            return minVar;
        };

        vector<Var> result;
        while (remains.size() > 0) {
            Var var = findMinDegreeVar(remains);
            result.push_back(var);
            remains.erase(var);
            for (Var var_n : neighbors.at(var)) { degrees[var_n] -= 1; }
        }
        return result;
    };

    map<Var, int> varColor;
    int           numColor = 0;
    // vars which have inference edges
    vector<Var>           kempeVarsSeq = kempe_simplify(infEdges);
    map<Var, vector<Var>> infNeighbors;
    for (const auto & [ u, v ] : infEdges) {
        assert(int(u) < int(v));
        infNeighbors[u].push_back(v);
        infNeighbors[v].push_back(u);
    }
    for (Var var : reverse(kempeVarsSeq)) {
        set<int> colors_n;
        for (Var var_n : infNeighbors[var]) {
            if (varColor.count(var_n)) { colors_n.insert(varColor.at(var_n)); }
        }
        int color = 1;
        while (colors_n.count(color) != 0) { color++; }
        varColor[var] = color;
        numColor      = max(numColor, color);
        logger.debug("kempe: {}: color {}", var_name(var), color);
    }
    // vars which have no inference edges
    for (const auto &var : vars) {
        if (varColor.count(var) == 0) {
            varColor[var] = 1;
            numColor      = max(numColor, 1);
        }
    }

    vector<string> varColorStrs =
        apply_map(to_vector(varColor), [](pair<Var, int> p) -> string {
            return var_name(p.first) + "=" + to_string(p.second);
        });
    logger.debug("  {}", fmt::join(varColorStrs, " "));
    logger.debug("coloring complete ({} colors)", numColor);

    assert(varColor.size() == vars.size());
    return varColor;
}

void assignVarsWithRegister(const Code &code) {
    auto[blocks, blockEdges] = getBasicBlocks(code);

    auto[globalVars, globalInfEdges] = getGlobalInfGraph(blocks, blockEdges);
    map<Var, int> globalVarColors    = coloring(globalVars, globalInfEdges);

    vector<map<Var, int>> localVarColors;
    vector<set<Var>>      localVars;
    for (const auto &block : blocks) {
        auto[localVars_elem, localInfEdges] =
            getLocalInfGraph(block, globalVars);
        auto varColors = coloring(localVars_elem, localInfEdges);
        localVars.push_back(localVars_elem);
        localVarColors.push_back(varColors);
    }

    auto getNumColors = [](const map<Var, int> &varColors) -> int {
        int numColors = 0;
        for (const auto & [ var, color ] : varColors) {
            numColors = max(numColors, color);
        }
        return numColors;
    };

    int           numGlobalVarColors = getNumColors(globalVarColors);
    map<Var, int> colorMapping;

    logger.info("global variables colors(num={})",
                getNumColors(globalVarColors));
    for (auto[var, color] : globalVarColors) {
        colorMapping[var] = color;
        logger.info("  {}: {}", var_name(var), color);
    }
    for (int i = 0; i < blocks.size(); i++) {
        logger.info("{} local variables colors(num={})",
                    lbl_name(blocks[i].lbl), getNumColors(localVarColors[i]));
        for (auto[var, color] : localVarColors[i]) {
            colorMapping[var] = numGlobalVarColors + color;
            logger.info("  {}: {}", var_name(var), numGlobalVarColors + color);
        }
    }

    const string regName[19] = {"$t0", "$t1", "$t2", "$t3", "$t4", "$t5",
                                "$t6", "$t7", "$t8", "$t9", "$s0", "$s1",
                                "$s2", "$s3", "$s4", "$s5", "$s6", "$s7"};
    for (auto[var, color] : colorMapping) {
        // 1-7: $t1 - $t7, 8-9: $t8 - $t9, 10- 17: $s0 - $s7
        // remain $t0 for immediate load use
        assert(1 <= color && color <= 17);
        varToReg[var] = regName[color];
    }
    varToReg[var_zero] = "$zero";
}

void genFuncBegin(const QuadTuple &q) {
    auto lbl      = q.args.func;
    auto info     = funcInfo.at(lbl);
    auto funcname = lblDecls.at(lbl)->lblname;
    int  spOffset = info.spOffset;

    // 8($fp) = param<1> (x)
    // 4($fp) = param<2> (y)
    // 0($fp) = $ra
    // -4($fp) = $fp_last
    // $fp - 8 = $sp
    genLabel(funcname);
    genCode("addiu", "$sp", "$fp", spOffset);

    // gen comments for offset meanings
    {
        const auto &    funcDecl = *funcDecls.at(lbl);
        vector<VarDecl> localVars;
        Appender{localVars}
            .append(funcDecl.params)
            .append(funcDecl.ret)
            .append(funcDecl.localVars);
        for (const auto &decl : localVars) {
            auto varname  = decl.varname;
            auto var      = decl.var;
            auto fpOffset = varInfo.at(var).fpOffset.value();

            genComment(
                to_string(fmt::format("{}: {}($fp)", varname, fpOffset)));
        }
    }
}

void genFuncRet(const QuadTuple &q) {
    // pop $sp, $ra, $fp
    genCode("ori", "$sp", "$fp", 0);
    genCode("lw", "$ra", "$fp", 0); // if no func call inside, can be ignored
    genCode("lw", "$fp", "$fp", -4);
    genCode("jr", "$ra");
    genCode("nop");
}

void genFuncEnd(const QuadTuple &q) { genCode("nop"); }

void genFuncCall(const QuadTuple &q) {
    auto lbl      = q.args.func;
    auto info     = funcInfo.at(lbl);
    auto funcname = lblDecls.at(lbl)->lblname;

    // assume parameters already pushed before
    genCode("addiu", "$sp", "$sp", -4 * info.numParams);

    genComment(string{"call function "} + funcname + " begin");
    // push $sp, $ra, $fp
    genCode("sw", "$ra", "$sp", 0);
    genCode("sw", "$fp", "$sp", -4);
    genCode("ori", "$fp", "$sp", 0);

    genCode("jal", funcname);
    genCode("nop");

    genCode("lw", "$ra", "$sp", 0);
    genCode("addiu", "$sp", "$sp", +4 * info.numParams);
    genComment(string{"call function "} + funcname + " end");
}

void genExprCode(const QuadTuple &q) {
    // r-type
    auto var_src1  = q.args.src1;
    auto var_src2  = q.args.src2;
    auto var_dest  = q.args.dest;
    auto info_src1 = varInfo.at(var_src1);
    auto info_src2 = varInfo.at(var_src2);
    auto info_dest = varInfo.at(var_dest);

    string reg_dest = varToReg.at(var_dest);

    // bool isImmediate =
    //     info_src1.constVal.has_value() || info_src2.constVal.has_value();
    bool isOffset =
        (info_src1.fpOffset.has_value() || info_src1.memName.has_value()) ||
        (info_src2.fpOffset.has_value() || info_src2.memName.has_value());
    bool isReg = varToReg.count(q.args.src1) || varToReg.count(q.args.src2);

    if (isReg && isOffset) {
        // reg1 + offset2
        if (info_src1.fpOffset.has_value() || info_src1.memName.has_value()) {
            swap(var_src1, var_src2);
            swap(info_src1, info_src2);
        }
        assert(q.op == Op::kAdd);
        assert(info_src2.fpOffset.has_value() || info_src2.memName.has_value());
        string reg_src1 = varToReg.at(var_src1);

        if (info_src2.fpOffset.has_value()) {
            int32_t fpOffset = info_src2.fpOffset.value();
            genCode("addiu", reg_dest, "$fp", fpOffset);
            genCode("addu", reg_dest, reg_dest, reg_src1);
        } else if (info_src2.memName.has_value()) {
            string memName = info_src2.memName.value();
            genCode("addiu", reg_dest, reg_src1, memName);
        } else {
            assert(false);
        }
    } else if (isReg) {
        // reg1 + reg2
        string reg_src1 = varToReg.at(var_src1);
        string reg_src2 = varToReg.at(var_src2);
        if (q.op == Op::kAdd) { genCode("addu", reg_dest, reg_src1, reg_src2); }
        if (q.op == Op::kMinus) {
            genCode("subu", reg_dest, reg_src1, reg_src2);
        }
        if (q.op == Op::kMult) {
            genCode("mult", reg_src1, reg_src2);
            genCode("mflo", reg_dest);
        }
        if (q.op == Op::kDiv) {
            genCode("div", reg_src1, reg_src2);
            genCode("mflo", reg_dest);
        }
        if (q.op == Op::kMod) {
            genCode("div", reg_src1, reg_src2);
            genCode("mfhi", reg_dest);
        }
        if (q.op == Op::kAnd) { genCode("and", reg_dest, reg_src1, reg_src2); }
        if (q.op == Op::kOr) { genCode("or", reg_dest, reg_src1, reg_src2); }
        if (q.op == Op::kXor) { genCode("xor", reg_dest, reg_src1, reg_src2); }
        if (q.op == Op::kNor) { genCode("nor", reg_dest, reg_src1, reg_src2); }
        if (q.op == Op::kLShift) {
            genCode("sllv", reg_dest, reg_src1, reg_src2);
        }
        if (q.op == Op::kRShift) {
            genCode("srlv", reg_dest, reg_src1, reg_src2);
        }
        if (q.op == Op::kEq) {
            genCode("xor", reg_dest, reg_src1, reg_src2);
            genCode("slti", reg_dest, reg_dest, "1");
        }
        if (q.op == Op::kNeq) {
            genCode("xor", reg_dest, reg_src1, reg_src2);
            genCode("sltiu", reg_dest, reg_dest, "1");
            genCode("sltiu", reg_dest, reg_dest, "1");
        }
        if (q.op == Op::kLeq) {
            genCode("subu", reg_dest, reg_src1, reg_src2);
            genCode("slti", reg_dest, reg_dest, "1");
        }
        if (q.op == Op::kLt) { genCode("slt", reg_dest, reg_src1, reg_src2); }
    } else {
        logger.error("{}", to_string(q));
        assert(false);
    }
}

void genLoadStoreCode(const QuadTuple &q) {
    /* (var_m, addr_m) */
    auto   v_var     = q.args.var_m;
    auto   v_addr    = q.args.addr_m;
    auto   reg_var   = varToReg.at(v_var);
    auto   info_addr = varInfo.at(v_addr);
    string op;

    if (q.op == Op::kLoad) {
        op = "lw";
    } else if (q.op == Op::kStore) {
        op = "sw";
    } else {
        assert(false);
    }

    if (info_addr.fpOffset.has_value()) {
        genCode(op, reg_var, "$fp", info_addr.fpOffset.value());
    } else if (info_addr.memName.has_value()) {
        // genCode(op, reg_var, "0", info_addr.memName.value());
        genOffsetCode(op, reg_var, "$zero", info_addr.memName.value());
    } else if (varToReg.count(v_addr) > 0) {
        auto reg_addr = varToReg.at(v_addr);
        genCode(op, reg_var, reg_addr, 0);
    } else {
        assert(false);
    }
}

void genRetPut(const QuadTuple &q) {
    /* (var_r, argc) */
    Var    var     = q.args.var_r;
    string reg_var = varToReg.at(var);
    int    idx     = q.args.argc;
    assert(idx == 0);

    genCode("ori", "$v0", reg_var, 0);
}

void genRetGet(const QuadTuple &q) {
    /* (var_r, argc) */
    Var    var     = q.args.var_r;
    string reg_var = varToReg.at(var);
    int    idx     = q.args.argc;
    assert(idx == 0);

    genCode("ori", reg_var, "$v0", 0);
}

void genParamPut(const QuadTuple &q) {
    /* (var_r, argc) */
    // # before func call
    // sw $5, 0($sp)  # %5 -> ParamPut<0>
    // sw $6, -4($sp) # %6 -> ParamPut<1>
    // sw $5, -8($sp) # %5 -> ParamPut<2>
    // sw $8, -8($fp) # write value(z) info mem(z)
    // add $sp, $sp, -12 # sp += 12

    // sw $ra, 0($sp) # call func2 (begin)
    // sw $fp, -4($sp)   # |
    // move $fp, $sp     # |
    // add $sp, $sp, -8  # |
    // jal func2         # |
    // lw $ra, 0($sp)    # | recover $ra
    // add $sp, $sp, 12  # | sp -= 12
    // nop              # call func2 (end)
    // # $fp, $sp should be recoverd

    Var    var_src = q.args.var_r;
    int    idx     = q.args.argc;
    string reg_src = varToReg.at(var_src);
    // TODO: pass param<0-3> by register
    // if (idx <= 3) {
    //     string reg_dest = "$v" + to_string(idx);
    //     genCode("addu", reg_dest, "$zero", reg_src);
    // } else {
    //     genCode("addu", reg_dest, "$zero", reg_src);
    // }
    genCode("sw", reg_src, "$sp", -4 * idx);
}

void genBranch(const QuadTuple &q) {
    /* (var_j, addr1, addr2) */
    Var    var       = q.args.var_j;
    Lbl    addr1     = q.args.addr1;
    Lbl    addr2     = q.args.addr2;
    string reg_var   = varToReg.at(var);
    string addrname1 = lblDecls.at(addr1)->lblname;
    string addrname2 = lblDecls.at(addr2)->lblname;

    genCode("bne", varToReg.at(var), "$zero", addrname1);
    genCode("nop");
    genCode("j", addrname2);
    genCode("nop");
}

string to_hex(int16_t src) {
    stringstream ss;
    ss << std::hex << std::showbase << src;
    return ss.str();
}

int32_t zero_extend(int16_t src) {
    return static_cast<uint16_t>(src);
}

int32_t sign_extend(int16_t src) {
    return static_cast<int16_t>(src);
}

// assign const value by immediate
void genAssign(const QuadTuple &q) {
    auto    var  = q.args.var;
    auto    reg  = varToReg.at(var);
    int32_t cval = q.args.cval;
    int16_t low  = cval & 0xFFFF;
    int16_t high = (cval >> 16) & 0xFFFF;

    if (zero_extend(low) == cval) {
        if (low < -100 || low > 100) {
            genCode("ori", reg, "$zero", to_hex(low));
        } else {
            genCode("ori", reg, "$zero", low);
        }
    } else if (sign_extend(low) == cval) {
        if (low < -100 || low > 100) {
            genCode("addiu", reg, "$zero", to_hex(low));
        } else {
            genCode("addiu", reg, "$zero", low);
        }
    } else {
        genCode("lui", reg, to_hex(high));
        genCode("ori", reg, reg, to_hex(low));
    }
}

// data segment definition
void genGlobal(const QuadTuple &q) {
    /* (var_a, width, len) */
    auto        var     = q.args.var_a;
    auto        varname = varInfo.at(var).memName.value();
    const auto &decl    = *varDecls.at(var);

    genData(varname, decl.type.size());
}

// entry of the whole program
void genDirector() {
    Lbl lbl_main = get_function_by_name("main").funcLbl.lbl;
    genFuncCall(QuadTuple{Op::kCall, {.func = lbl_main, .argc = 0}});
    // call main
    genCode("nop");
}

// dispatcher
void genCodes(const QuadTuple &q) {
    switch (q.op) {
        case Op::kNop:
            genCode("nop");
            break;
        case Op::kBackPatch:
            assert(false);
            break;
        case Op::kAssign:
            genAssign(q);
            break;
        case Op::kAdd:
        case Op::kMinus:
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
            genExprCode(q);
            break;
        case Op::kAllocate:
            // should not appear
            assert(false);
            // but also need no to do
            // pass
            break;
        case Op::kGlobal:
            genGlobal(q);
            break;
        case Op::kLoad:
        case Op::kStore:
            genLoadStoreCode(q);
            break;
        case Op::kParamPut:
            genParamPut(q);
            break;
        case Op::kParam:
            // pass
            break;
        case Op::kRetPut: // put
            genRetPut(q);
            break;
        case Op::kRetGet: // set
            genRetGet(q);
            break;
        case Op::kRet:
            genFuncRet(q);
            break;
        case Op::kCall:
            genFuncCall(q);
            break;
        case Op::kLabel:
            genLabel(lblDecls.at(q.args.addr1)->lblname);
            break;
        case Op::kGoto:
            genCode("j", lblDecls.at(q.args.addr1)->lblname);
            genCode("nop");
            break;
        case Op::kBranch:
            genBranch(q);
            break;
        case Op::kFuncBegin:
            genFuncBegin(q);
            break;
        case Op::kFuncEnd:
            genFuncEnd(q);
            break;
        default:
            assert(false);
    }
}

extern void varsNamingTest() {
    // initVarInfo();
    for (const auto &funcDecl : globalFuncDecls) {
        logger.info("in function {}:", lbl_name(funcDecl.funcLbl.lbl));
        const auto &code = funcDecl.code;
        assignVarsWithRegister(code);
    }
}

extern string getMipsCodeStr() {
    vector<QuadTuple> irDataCodes;
    vector<QuadTuple> irTextCodes;
    for (const auto &decl : globalVarDecls) {
        irDataCodes.push_back(gen_allocate_code(decl, Op::kGlobal));
    }
    for (const auto &decl : globalFuncDecls) {
        Appender{irTextCodes}.append(decl.code);
    }

    genSection(".DATA");
    for (const auto &ir : irDataCodes) { genCodes(ir); }
    genSection(".TEXT");
    genDirector(); // direct to main
    for (const auto &ir : irTextCodes) { genCodes(ir); }

    string mipsCodeStr = to_string(fmt::format("{}", fmt::join(
        mipsCode, "\n")));
    return mipsCodeStr;
}