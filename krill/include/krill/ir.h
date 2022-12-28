#ifndef IR_H
#define IR_H
#include "defs.h"
#include "lexical.h"
#include "syntax.h"
#include <deque>
#include <memory>
#include <ostream>
#include <stack>
#include <string>
#include <optional>
using krill::runtime::SyntaxParser, krill::runtime::LexicalParser;
using krill::type::Grammar;

namespace krill::ir {

struct Lbl {
    int id_;
    Lbl() = default;
    constexpr Lbl(int id) : id_(id){};
         operator int() const { return id_; };
    Lbl &operator++(int) {
        id_++;
        return *this;
    };
};

struct Var {
    int id_;
    Var() = default;
    constexpr Var(int id) : id_(id){};
         operator int() const { return id_; };
    Var &operator++(int) {
        id_++;
        return *this;
    };
    bool operator<=(const Var &var) const {
        return id_ <= var.id_;
    }
    bool operator<(const Var &var) const {
        return id_ < var.id_;
    }
    bool operator==(const Var &var) const {
        return id_ == var.id_;
    }
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
            Var     var;
            int32_t cval;
        }; // assign
        struct {
            Var var_a;
            int size;
        }; // alloca,  global
        struct {
            Var dest;
            Var src1;
            Var src2;
        }; // 2-op / 1-op calculate
        struct {
            Var var_m;
            Var addr_m;
        }; // mem load/store
        struct {
            Var var_j;
            Lbl addr1;
            Lbl addr2;
        }; // jump, branch
        struct {
            Var var_r;
            int argc;
            Lbl func;
        };
    } args;
};

using Code = vector<QuadTuple>;

constexpr Var var_zero  = {0};
constexpr Var var_empty = {};
constexpr Lbl lbl_empty = {};

enum class TypeSpec { kVoid, kInt32 };
enum class VarDomain { kLocal, kGlobal };

struct TypeDecl {
    TypeSpec    basetype;
    vector<int> shape;
    
    int size() const {
        int size_ = (basetype == TypeSpec::kInt32) ? 4 : 0;
        for (int len : shape) { size_ *= len; }
        return size_;
    };
    bool operator==(const TypeDecl &ts) const {
        return std::tie(basetype, shape) == std::tie(ts.basetype, ts.shape);
    }
    bool operator!=(const TypeDecl &ts) const {
        return std::tie(basetype, shape) != std::tie(ts.basetype, ts.shape);
    }
};

struct VarDecl {
    VarDomain domain;
    TypeDecl  type;
    string    varname;
    Var       var;
};

struct LblDecl {
    string lblname;
    Lbl    lbl;
};

struct FuncDecl {
    // TODO: add type-only attribute for supporting overlode
    // (locating a func-decl by its name together with its params type)
    // vector<TypeDecl> paramsType;
    // TypeDecl         retType;
    LblDecl         funcLbl;
    vector<VarDecl> params;
    vector<VarDecl> localVars;
    vector<LblDecl> localLbls;
    vector<VarDecl> ret;
    string          funcName;
    Code            code;
};

struct VarInfo {
    // exclusive
    std::optional<int>     constVal; // it's a const value
    std::optional<int>     fpOffset; // it's a offset relatived to $fp
    std::optional<string>  memName;  // it's a data segment label

    std::optional<string>  reg;      // assigned register
};

struct FuncInfo {
    string name;
    int spOffset = 0; // local space
    int numParams;
};

// all you need to do is using this lovely interface
string genMipsCodes(istream iss);

} // namespace krill::ir

#endif