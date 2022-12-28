#include "fmt/format.h"
#include "krill/minic.h"
#include "krill/ir.h"
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
using namespace krill::ir;

using krill::log::logger;


extern vector<FuncDecl> globalFuncDecls;
extern vector<VarDecl>  globalVarDecls;

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


// additional information generated in this procedure
map<Var, VarInfo>  varInfo;
map<Lbl, FuncInfo> funcInfo;

void initVarInfo() {
    varInfo  = {};
    funcInfo = {};

    int memOffset = 0;
    // initlaize zero variable
    varInfo[var_zero] = VarInfo{.constVal = {0}};
    // initialize global variable info
    for (const auto &decl : globalVarDecls) {
        varInfo[decl.var] =
            VarInfo{.memName = {decl.varname}};
        memOffset += decl.type.size();
    }

    // initialize local variable info
    for (auto &funcDecl : globalFuncDecls) {
        /**
         * e.g.: int func(int x, int y[10], int z);
         * given:
         *     12($fp) = param<0> (int32 x)
         *      8($fp) = param<1> (int32* y)
         *      4($fp) = param<2> (int32 z)
         *      0($fp) = $ra
         *     -4($fp) = $fp_last
         *        $sp  = $fp - 8
         * do:
         *     -8($fp) <- local_var<0> (int32 retval)
         *    -12($fp) <- local_var<3> (int32[10] z)
         *    -52($fp) <- local_var<4> (int32 a)
         *        $sp  <- $fp - 48
         **/

        // assign local stack space for parameters
        // e.g. 12($fp) for param<0>, 8($fp) for param<1>, 4($fp) for param<0>
        int paramOffset = 4 * funcDecl.params.size();
        for (const auto &decl : funcDecl.params) {
            varInfo[decl.var] =
                VarInfo{.fpOffset = paramOffset};
            paramOffset -= 4;
        }

        // reverse 0($fp) for $fp, -4($fp) for $sp, -8($fp) for $ra
        // -12($fp) for retval (if there is), -16($fp) ... for local variables
        int spOffset = -8;
        for (const auto &decl : funcDecl.ret) {
            varInfo[decl.var] =
                VarInfo{.fpOffset = {spOffset}};
            spOffset -= decl.type.size(); // allocate actual spcae for variables
        }
        for (const auto &decl : funcDecl.localVars) {
            varInfo[decl.var] =
                VarInfo{.fpOffset = {spOffset}};
            spOffset -= decl.type.size(); // allocate actual spcae for variables
        }

        // set $sp
        Lbl lbl = funcDecl.funcLbl.lbl;
        funcInfo[lbl] = {.spOffset  = spOffset,
                         .numParams = int(funcDecl.params.size())};


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
