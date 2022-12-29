#ifndef MINIC_H
#define MINIC_H
#include "defs.h"
#include <deque>
#include <list>
#include <memory>
#include <optional>
#include <ostream>
#include <stack>
#include <string>
#include <tuple>
using krill::type::Grammar;

namespace krill::ir {

// variable (temporal, local, global)
// only store context-free infomation
enum class TypeSpec { kVoid, kInt32 };

struct Var {
    struct TypeDecl {
        TypeSpec    basetype;
        vector<int> shape;

        int size() const;
        string str() const;
        bool   operator==(const TypeDecl &ts) const;
        bool   operator!=(const TypeDecl &ts) const;
        bool   operator<(const TypeDecl &ts) const;
    };
    struct Info {
        // exclusive
        std::optional<int>    constVal; // it's a const value
        std::optional<int>    fpOffset; // it's a offset relatived to $fp
        std::optional<string> memName;  // it's a data segment label

        std::optional<string> reg; // assigned register
    };

    string   name;
    TypeDecl type;
    // for optimization use
    VarInfo info;
};

// label: indicate a position of the code (be inserted into)
struct Lbl {
    string name;
};

// function
struct Func {
    struct TypeDecl {
        string                funcName;
        vector<Var::TypeDecl> returnsType;
        vector<Var::TypeDecl> paramsType;

        string str() const;
        bool   operator==(const TypeDecl &ts) const;
        bool   operator!=(const TypeDecl &ts) const;
        bool   operator<(const TypeDecl &ts) const;
    };

    struct Info {
        string name;
        int    spOffset = 0; // local space
        int    numParams;
    };

    string        name;
    vector<Var *> params;
    vector<Var *> returns;
    // assigned after definition occur
    optional<Code> code; // if has no value, require to be LINKed!
    vector<Var *>  localVars;
    // for optimization use
    FuncInfo info;

    TypeDecl type() const;
};

enum class Op {
    // clang-format off
    kNop, 
    kAssign, /* .args_i (var, cval) */
    kAdd, kMinus, kMult, kDiv, kMod,/* .args_e (dest, src1, src2) */
    kAnd, kOr, kXor, kNor,          /* .args_e (dest, src1, src2) */
    kLShift, kRShift,               /* .args_e (dest, src1, src2) */
    kEq, kNeq, kLeq, kLt,           /* .args_e (dest, src1, src2) */
    kAlloca,  kGlobal,     /* .args_d (var, size) */
    kLoad, kStore,  /* .args_m (var, mem) */
    kRet,    /* () */
    kLabel,  /* .args_j (addr1) */
    kGoto,   /* .args_j (addr1) */
    kBranch, /* .args_j (var, addr1, addr2) */
    kCall,   /* .args_f (func) */
    kParamPut, kRetPut, kRetGet /* .args_f (var, idx) */ 
    kFuncBegin, /* .args_f (func) */
    kFuncEnd,
    // clang-format on
};

// intermediate code (quad-tuple)
struct QuadTuple {
    Op op;
    union {
        struct { // immediate: assign
            Var *   var;
            int32_t cval;
        } args_i;
        struct { // declaration: allocate,  global
            Var *   var;
            int32_t size;
        } args_d;
        struct { // expresstion: 2-op / 1-op calculate
            Var *dest;
            Var *src1;
            Var *src2;
        } args_e;
        struct { // memory: mem load/store
            Var *var;
            Var *mem;
        } args_m;
        struct { // jump: jump, branch
            Var *var;
            Lbl *addr1;
            Lbl *addr2;
        } args_j;
        struct { // function: call / param / RetPut
            Var *   var;
            Func *  func;
            int32_t idx;
        } args_f;
    };
};

using Code = vector<QuadTuple>;

constexpr Var        var_zero_base = {.name = "zero"};
constexpr const Var *var_zero      = &var_zero_base;

// only for livespan management, can be redundant
vector<unique_ptr<Var>>  variables_pool;
vector<unique_ptr<Lbl>>  labels_pool;
vector<unique_ptr<Func>> functions_pool;

Var * assign_new_variable(const Var &base);
Lbl * assign_new_label(const Lbl &base);
Func *assign_new_function(const Func &base);

// parsed result 
vector<Func *> globalFuncs;
vector<Var *>  globalVars;

} // namespace krill::ir

#endif