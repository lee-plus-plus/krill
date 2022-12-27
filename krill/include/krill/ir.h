#ifndef MINIC_H
#define MINIC_H
#include "defs.h"
#include <deque>
#include <memory>
#include <ostream>
#include <stack>
#include <string>
#include <optional>
#include <list>
#include <tuple>
using krill::type::Grammar;

namespace krill::ir {

// type of a variable, (int, int[10][20], void, ...)
enum class TypeSpec { kVoid, kInt32 };
struct TypeDecl {
    TypeSpec    basetype;
    vector<int> shape;
    
    constexpr int size() const;
    bool operator==(const TypeDecl &ts) const;
    bool operator!=(const TypeDecl &ts) const;
};

// variable (temporal, local, global)
// only store context-free infomation
struct Var {
    struct Info {
        // exclusive
        std::optional<int>     constVal; // it's a const value
        std::optional<int>     fpOffset; // it's a offset relatived to $fp
        std::optional<string>  memName;  // it's a data segment label

        std::optional<string>  reg;      // assigned register
    };

    string   name;
    TypeDecl type;
    VarInfo  info;
};

// label: indicate a position of the code (be inserted into)
struct Lbl {
    string name;
};

// function
struct Func {
    struct Info {
        string name;
        int spOffset = 0; // local space
        int numParams;
    };

    string        name;
    vector<Var *> params;
    vector<Var *> returns;
    // instantiate later
    vector<Var *> localVars;
    Lbl *         lbl;
    FuncInfo      info;

    using FuncTypeDecl = tuple<string, vector<TypeDecl>,
                               vector<TypeDecl>>; // <name, ret, params>
    FuncType getTypeDecl() const;
};

enum class Op {
    // clang-format off
    kNop, 
    kBackPatch, /* (func, argc) */
    kAssign, /* (var, cval) */
    kAdd, kMinus, kMult, kDiv, kMod,/* (dest, src1, src2) */
    kAnd, kOr, kXor, kNor,          /* (dest, src1, src2) */
    kLShift, kRShift,               /* (dest, src1, src2) */
    kEq, kNeq, kLeq, kLt,           /* (dest, src1, src2) */
    kAllocate,  kGlobal,     /* (var_a, width, len) */
    kLoad, kStore,  /* (var_m, addr_m) */
    kParamPut, kParam, /* (var_r, argc) */ 
    kRetPut, kRetGet, /* (var_r, argc) */
    kRet,    /* () */
    kCall,   /* (func, argc) */
    kLabel,  /* (addr1) */
    kGoto,   /* (addr1) */
    kBranch, /* (var_j, addr1, addr2) */
    kFuncBegin, /* (func) */
    kFuncEnd, 
    // clang-format on
};

// intermediate code (quad-tuple)
struct QuadTuple {
    Op op;
    union Data { // the following code use clang-only feature
        struct {
            Var *   var;
            int32_t cval;
        }; // assign
        struct {
            Var *var_a;
            int size;
        }; // alloca,  global
        struct {
            Var *dest;
            Var *src1;
            Var *src2;
        }; // 2-op / 1-op calculate
        struct {
            Var *var_m;
            Var *addr_m;
        }; // mem load/store
        struct {
            Var *var_j;
            Lbl *addr1;
            Lbl *addr2;
        }; // jump, branch
        struct {
            Var  *var_r;
            int   argc;
            Func *func;
        }; // call / paramPut / param / RetPut / RetGet
    } args;
};

using Code = vector<QuadTuple>;

constexpr Var var_zero  = {0};
constexpr Var var_empty = {};
constexpr Lbl lbl_empty = {};

// global data
vector<unique_ptr<Var>>  variables;
vector<unique_ptr<Lbl>>  labels;
vector<unique_ptr<Func>> functions;

} // namespace krill::minic::ir

#endif