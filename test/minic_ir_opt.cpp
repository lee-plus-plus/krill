#include "fmt/format.h"
#include "krill/minic.h"
#include "krill/utils.h"
#include <cstdlib>
#include <iostream>
#include <map>
#include <sstream>
#include <vector>
using namespace std;
using namespace krill::type;
using namespace krill::utils;
using namespace krill::minic;
using namespace krill::minic::ir;

using krill::log::logger;

extern string lbl_name(const Lbl &lbl);
extern string var_name(const Var &var);

bool isOpExpr(Op op) {
    switch (op) {
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
            return true;
        default:
            return false;
    }
}

int calcExpr(Op op, int val1, int val2) {
    switch (op) {
        case Op::kAdd:
            return val1 + val2;
        case Op::kMinus:
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
    }
}

// vector<DagNode *>   nodes;
// map<Var, DagNode *> varToNodes;
// map<int, DagNode *> cvalTolNodes;

// additional information generated in this procedure
MemInfo            memInfo;
map<Var, VarInfo>  varInfo;
map<Lbl, FuncInfo> funcInfo;

extern vector<FuncDecl> globalFuncDecls;
extern vector<VarDecl>  globalVarDecls;
extern string           lbl_name(const Lbl &lbl);
extern string           var_name(const Var &var);
extern QuadTuple        gen_allocate_code(const VarDecl &decl, const Op &op);

void initVarInfo() {
    varInfo  = {};
    funcInfo = {};
    memInfo  = {};

    int memOffset = 0;
    // initlaize zero variable
    varInfo[var_zero] = VarInfo{.constVal = {0}};
    // initialize global variable info
    for (const auto &decl : globalVarDecls) {
        varInfo[decl.var] =
            VarInfo{.name = {decl.varname}, 
                    .memName = {decl.varname}};
        memOffset += decl.type.size;
    }
    memInfo.memOffset = memOffset;

    // initialize local variable info
    for (auto &funcDecl : globalFuncDecls) {
        // e.g.: int func(int x, int y[10], int z);
        // given:
        // 12($fp) = param<0> (int32 x)
        //  8($fp) = param<1> (int32* y)
        //  4($fp) = param<2> (int32 z)
        //  0($fp) = $ra
        // -4($fp) = $fp_last
        //    $sp  = $fp - 8
        // do:
        //  -8($fp) <- local_var<0> (int32 retval)
        // -12($fp) <- local_var<3> (int32[10] z)
        // -52($fp) <- local_var<4> (int32 a)
        //     $sp  <- $fp - 48

        // assign local stack space for parameters
        // if 3 params: 12($fp) for param<0>, 8($fp) for param<1>, 4($fp) for
        // param<2>
        int paramOffset = 4 * funcDecl.params.size();
        for (const auto &decl : funcDecl.params) {
            varInfo[decl.var] =
                VarInfo{.name = decl.varname, .fpOffset = paramOffset};
            paramOffset -= 4;
        }

        // reverse 0($fp) for $fp, -4($fp) for $sp, -8($fp) for $ra
        // assign local stack for return value, , local variables
        int spOffset = -8;
        for (const auto &decl : funcDecl.ret) {
            varInfo[decl.var] =
                VarInfo{.name = decl.varname, .fpOffset = {spOffset}};
            spOffset -= decl.type.size; // allocate actual spcae for variables
        }
        for (const auto &decl : funcDecl.localVars) {
            varInfo[decl.var] =
                VarInfo{.name = decl.varname, .fpOffset = {spOffset}};
            spOffset -= decl.type.size; // allocate actual spcae for variables
        }
        // set $sp
        funcInfo[funcDecl.funcLbl.lbl] = {.name     = funcDecl.funcName,
                                          .spOffset = spOffset,
                                          .numParams =
                                              int(funcDecl.params.size())};


        // assign const value for temporal variables
        map<Var, QuadTuple *> cvalCode;

        for (auto &q : funcDecl.code) {
            // set var info
            if (q.op == Op::kAllocate || q.op == Op::kParam) {
                // pass
                q = QuadTuple{.op = Op::kNop}; // delete
            }
            if (q.op == Op::kAssign) {
                varInfo[q.args.var] = VarInfo{.constVal = {q.args.cval}};
                // q = QuadTuple{.op = Op::kNop}; // delete
                cvalCode[q.args.var] = &q;
            } else if (q.op == Op::kLoad) {
                varInfo[q.args.var_m] = VarInfo{};
            } else if (isOpExpr(q.op)) {
                varInfo[q.args.var_m] = VarInfo{};
            } else if (q.op == Op::kRetGet) {
                varInfo[q.args.var_m] = VarInfo{};
            }

            // const value propagation
            if (isOpExpr(q.op)) {
                // const value propagation
                const Var  var_src1 = q.args.src1;
                const Var  var_src2 = q.args.src2;
                const Var  var_dest = q.args.dest;
                const auto src1   = varInfo.at(var_src1);
                const auto src2   = varInfo.at(var_src2);
                auto &     dest   = varInfo.at(var_dest);

                if (src1.constVal.has_value() && src2.constVal.has_value()) {
                    int cval1     = src1.constVal.value();
                    int cval2     = src2.constVal.value();
                    int result    = calcExpr(q.op, cval1, cval2);
                    dest.constVal = {result};
                    // overwrite
                    q = QuadTuple{Op::kAssign,
                                  {.var = var_dest, .cval = result}}; 

                    cvalCode[var_dest]     = &q;
                    *cvalCode.at(var_src1) = QuadTuple{.op = Op::kNop};
                    *cvalCode.at(var_src2) = QuadTuple{.op = Op::kNop};
                }
            }
        }

        funcDecl.code = apply_filter(funcDecl.code, [](QuadTuple q) -> bool {
            return q.op != Op::kNop;
        });
    }
}

string var_name2(Var var) {
    auto info = varInfo.at(var);
    // if (info.constVal.has_value()) { return to_string(info.constVal.value()); }
    if (info.fpOffset.has_value()) {
        return string{"@"} + info.name + string{": "} +
               to_string(info.fpOffset.value()) + "($fp)";
    }
    if (info.memName.has_value()) {
        return string{"@"} + to_string(info.memName.value());
    }
    return string{"%"} + to_string(var);
}

string to_string2(const QuadTuple &q) {
    stringstream ss;
    switch (q.op) {
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
            ss << fmt::format("{} = {:s} {} {}", var_name2(q.args.var),
                              enum_name(q.op), var_name2(q.args.src1),
                              var_name2(q.args.src2));
            break;
        case Op::kGlobal:
            ss << fmt::format("{} = {:s}", var_name2(q.args.var_a),
                              enum_name(q.op));
            if (q.args.len == 0) {
                ss << fmt::format(" {:d}", int(q.args.width));
            } else {
                ss << fmt::format(" [{:d} x {:d}]", int(q.args.len),
                                  int(q.args.width));
            }
            break;
        case Op::kLoad:
            ss << fmt::format("{} = {:s} {}", var_name2(q.args.var_m),
                              enum_name(q.op), var_name2(q.args.addr_m));
            break;
        case Op::kStore:
            ss << fmt::format("{} = {:s} {}", var_name2(q.args.addr_m),
                              enum_name(q.op), var_name2(q.args.var_m));
            break;
        case Op::kParamPut:
            ss << fmt::format("{}<{:d}> {} ", enum_name(q.op), int(q.args.argc),
                              var_name2(q.args.var_r));
            break;
        case Op::kParam:
            ss << fmt::format("{} = {:s}<{:d}>", var_name2(q.args.var_r),
                              enum_name(q.op), int(q.args.argc));
            break;
        case Op::kRetPut:
            ss << fmt::format("{}<{:d}> {} ", enum_name(q.op), int(q.args.argc),
                              var_name2(q.args.var_r));
            break;
        case Op::kRetGet:
            ss << fmt::format("{} = {:s}<{:d}>", var_name2(q.args.var_r),
                              enum_name(q.op), int(q.args.argc));
            break;
        case Op::kRet:
            ss << fmt::format("{:s}<{:d}>", enum_name(q.op), int(q.args.argc));
            break;
        case Op::kCall:
            ss << fmt::format("{:s} {}<{:d}>", enum_name(q.op),
                              lbl_name(q.args.func), int(q.args.argc));
            break;
        case Op::kLabel:
            ss << fmt::format("{}:", lbl_name(q.args.addr1));
            break;
        case Op::kGoto:
            ss << fmt::format("{:s} {}", enum_name(q.op),
                              lbl_name(q.args.addr1));
            break;
        case Op::kBranch:
            ss << fmt::format("{:s} ({}) {} {}", enum_name(q.op),
                              var_name2(q.args.var_j), lbl_name(q.args.addr1),
                              lbl_name(q.args.addr2));
            break;
        case Op::kFuncBegin:
            ss << fmt::format("func {} {{", lbl_name(q.args.func));
            break;
        case Op::kFuncEnd:
            ss << fmt::format("}}");
            break;
        case Op::kAssign:
            ss << fmt::format("{} = {:s} {:d}", var_name2(q.args.var),
                              enum_name(q.op), int(q.args.cval));
            break;
        default:
            assert(false);
    }
    return ss.str();
}

string to_string2(const Code &code) {
    stringstream ss;
    for (const auto &q : code) {
        if (q.op == Op::kLabel || q.op == Op::kFuncEnd ||
            q.op == Op::kFuncBegin) {
            ss << to_string2(q) << "\n";
        } else {
            ss << "\t" << to_string2(q) << "\n";
        }
    }
    return ss.str();
}

string get_ir_str2() {
    stringstream ss;

    // print global variables declaration
    for (const auto &var_decl : globalVarDecls) {
        ss << to_string2(gen_allocate_code(var_decl, Op::kGlobal)) << "\n";
    }
    ss << "\n";

    // print global functions declaration
    for (const auto &f : globalFuncDecls) {
        for (const auto &q : f.code) {
            if (q.op == Op::kLabel || q.op == Op::kFuncEnd ||
                q.op == Op::kFuncBegin) {
                ss << to_string2(q) << "\n";
            } else {
                ss << "\t" << to_string2(q) << "\n";
            }
        }
    }
    return ss.str();
}

// DagNode *getOrNewNode(const Node &node, ) {
//     for (DagNode *it : nodes) {
//         if (*it == tgt) { return it; }
//     }
//     node = new DagNode{Node};
//     nodes.push_back(node);
//     return node;
// }


// struct DagNode {
//     Op            op   = Op::kNop;
//     DagNode *     src1 = nullptr;
//     DagNode *     src2 = nullptr;
//     optional<int> constVal;

//     bool used = false;

//     inline bool operator==(const DagNode &dn) const {
//         if (constVal.has_value() && dn.constVal.has_value()) {
//             return constVal.value() == dn.constVal.value();
//         }
//         return std::tie(op, src1, src2) == std::tie(dn.op, dn.src1, dn.src2);
//     }
// }; // comparable

// optimization1: const value propagation
/*
void optimization1(Code &code, const map<Var, *VarDecl> &varDecls) {
    // build DAG
    int fpOffset  = 0;
    int memOffset = 0;

    map<Var, Var> replaced;
    set<Var> removed;


    // build DAG, do const value propagation
    for (auto &q : code) {
        if (op == kNop || kBackPatch) {
            // pass
        }
        if (op == Op::kAssign) { // set
            // (var, cval)
            DagNode *node =
                getOrCreateNode({.constVal = q.args.cval, .var = q.args.var});
            varToNodes[q.args.var] = node;
            // delete code
            q = QuadTuple{.op = Op::kNop};
            // set var info
            varInfo[var] = {.constVal = {q.args.cval}};
        } else if (isOpExpr(op)) { // set, use
            // (dest, src1, src2)
            DagNode *src1 = varToNodes.at(q.args.src1);
            DagNode *src2 = varToNodes.at(q.args.src2);
            DagNode *node;
            if (src1.constVal.has_value() && src1.constVal.has_value()) {
                // const value propagation
                int cval1 = src1.constVal.value();
                int cval2 = src2.constVal.value();
                int result = calcExpr(op, cval1, cval2);
                node = getOrCreateNode({.constVal = result, .var = q.args.var});
                // delete code
                q = QuadTuple{.op = Op::kNop};
                // set var info
                varInfo[var] = {.constVal = {result}};
            } else {
                node = getOrCreateNode(
                    {.op = op, .src1 = src1, .src2 = src2, .var = q.args.dest});
                // replace used var
                q.args.src1 = src1->var;
                q.args.src2 = src1->var;
                // set var info
                varInfo[var] = {};
            }
            varToNodes[q.args.dest] = node;
        } else if (op == Op::kAllocate || op == Op::kGlobal) { // set
            // (var_a, width, len)
            int &    offset = (op == Op::kAllocate) ? fpOffset : memOffset;
            DagNode *node   = getOrCreateNode({.constVal = offset});
            offset -= varDecls.at(q.args.var_a)->type.size;
            varToNodes[q.args.var] = node;
            // set var info
            varInfo[var] = (op == Op::kAllocate) ? {.fpOffset = {offset}}
                                                 : {.memOffset = {offset}};
            }
        } else if (op == Op::kLoad) { // set, use
            // (var_m, addr_m)
            DagNode *node_addr_m = varToNodes.at(q.args.addr_m);
            DagNode *node = getOrCreateNode({.op = op, .src1 = node_addr_m});
            // replace used var
            q.args.addr_m = node_addr_m->var;
            varToNodes[q.args.var_m] = node;
        } else if (op == Op::kStore) { // use
            // (var_m, addr_m)
            DagNode *node_addr_m = varToNodes.at(q.args.addr_m);
            DagNode *node_var_m  = varToNodes.at(q.args.var_m);
            // replace used var
            q.args.addr_m = node_addr_m->var;
            q.args.addr_m = node_addr_m->var;
            node_addr_m->used    = true;
            node_var_m->used     = true;
        } else if (op == Op::kParamPut || op == Op::kRetPut) {
            // (var_r, argc)
            DagNode *node = varToNodes.at(q.args.var_r) node_addr_m->used =
                true;
        } else if (op == Op::kParamGet || op == Op::kRetGet) {
            // (var_r, argc)
            DagNode *node          = new Node{};
            varToNodes[q.args.var] = node;
        } else if (op == Op::kRet || op == Op::kCall) {
            // pass
        } else if (op == Op::kLabel || op == Op::kGoto || op == Op::kBranch) {
            // pass
        } else if (op == Op::kFuncBegin || op == Op::kFuncEnd) {
            // pass
        } else {
            assert(false);
        }
    }

    // eliminate unused variables

}
*/