#ifndef MINIC_H
#define MINIC_H
#include "defs.h"
#include "lexical.h"
#include "syntax.h"
#include <deque>
#include <memory>
#include <ostream>
#include <stack>
#include <string>
using krill::runtime::SyntaxParser, krill::runtime::LexicalParser;
using krill::type::Grammar;

namespace krill::minic {

extern Grammar       minicGrammar;
extern SyntaxParser  minicSyntaxParser;
extern LexicalParser minicLexicalParser;
extern int           getMinicSyntaxId(Token token);

} // namespace krill::minic

namespace krill::minic::syntax {

// Grammar
constexpr int ζ = -1;
constexpr int IDENT = 258;
constexpr int VOID = 259;
constexpr int INT = 260;
constexpr int WHILE = 261;
constexpr int IF = 262;
constexpr int ELSE = 263;
constexpr int RETURN = 264;
constexpr int EQ = 265;
constexpr int NE = 266;
constexpr int LE = 267;
constexpr int GE = 268;
constexpr int AND = 269;
constexpr int OR = 270;
constexpr int DECNUM = 271;
constexpr int CONTINUE = 272;
constexpr int BREAK = 273;
constexpr int HEXNUM = 274;
constexpr int LSHIFT = 275;
constexpr int RSHIFT = 276;
constexpr int program = 277;
constexpr int decl_list = 278;
constexpr int decl = 279;
constexpr int var_decl = 280;
constexpr int fun_decl = 281;
constexpr int type_spec = 282;
constexpr int int_literal = 283;
constexpr int FUNCTION_IDENT = 284;
constexpr int params = 285;
constexpr int compound_stmt = 286;
constexpr int param_list = 287;
constexpr int param = 288;
constexpr int stmt_list = 289;
constexpr int stmt = 290;
constexpr int expr_stmt = 291;
constexpr int block_stmt = 292;
constexpr int if_stmt = 293;
constexpr int while_stmt = 294;
constexpr int return_stmt = 295;
constexpr int continue_stmt = 296;
constexpr int break_stmt = 297;
constexpr int expr = 298;
constexpr int args_ = 299;
constexpr int WHILE_IDENT = 300;
constexpr int local_decls = 301;
constexpr int local_decl = 302;
constexpr int arg_list = 303;

} // namespace krill::minic::syntax

namespace krill::minic::ir {

// intermediate code (quad-tuple)
struct QuadTuple {
    struct Lbl {
        int data_;
        Lbl() = default;
        constexpr Lbl(int data) : data_(data){};
             operator int() const { return data_; };
        Lbl &operator++(int) {
            data_++;
            return *this;
        };
    };
    struct Var {
        int data_;
        Var() = default;
        constexpr Var(int data) : data_(data){};
             operator int() const { return data_; };
        Var &operator++(int) {
            data_++;
            return *this;
        };
        bool operator<=(const Var &var) const {
            return data_ <= var.data_;
        }
        bool operator<(const Var &var) const {
            return data_ < var.data_;
        }
        bool operator==(const Var &var) const {
            return data_ == var.data_;
        }
    };
    struct CVal {
        uint32_t data_;
        CVal() = default;
        constexpr CVal(int data) : data_(data){};
        operator int() const { return data_; };
    };
    // clang-format off
    enum class Op {
        kNop, 
        kBackPatch, /* (func, argc) */
        kAssign, /* (var, cval) */
        kCopy,   /* (var, src1) */
        kAdd, kMinus, kMult, kDiv, kMod,/* (dest, src1, src2) */
        kAnd, kOr, kXor, kNor,          /* (dest, src1, src2) */
        kLShift, kRShift,               /* (dest, src1, src2) */
        kEq, kNeq, kLeq, kLt,           /* (dest, src1, src2) */
        kAllocate,  kGlobal,     /* (var_a, width, len) */
        kLoad, kStore,  /* (var_m, addr_m) */
        kParamPut,  /* (var_r, argc) */ 
        kParamGet,  /* (var_r, argc) */ 
        kCall,   /* (func, argc, var_r) */
        kLabel,  /* (addr1) */
        kGoto,   /* (addr1) */
        kBranch, /* (var_j, addr1, addr2) */
        kRet,    /* (argc, var_r) */
        kFuncBegin, /* (addr1) */
        kFuncEnd, 
    };
    // clang-format on
    Op op;
    union Data {
        struct {
            Var  var;
            CVal cval;
        }; // assign
        struct {
            Var var_a;
            int width;
            int len; /* len > 0: is array */
        };           // alloate,  global
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
            Lbl func;
            int argc;
            Var var_r;
        };
    } args;
};

using Op   = QuadTuple::Op;
using Lbl  = QuadTuple::Lbl;  // label in TEXT section
using Var  = QuadTuple::Var;  // register / local / global
using CVal = QuadTuple::CVal; // const value in asm, uint32_t
using Code = vector<QuadTuple>;

constexpr Var var_zero  = {0};
constexpr Var var_empty = {};
constexpr Lbl lbl_empty = {};


enum class TypeSpec { kVoid, kInt32 };
enum class VarDomain { kLocal, kGlobal };
struct TypeDecl {
    TypeSpec    basetype;
    vector<int> shape;

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
    VarDecl         ret;
    string          funcName;
    Code            code;
};

// intermediate represetation
// extern struct MidRep {
//     vector<FuncDecl> globalFuncDecls;
//     vector<VarDecl>  globalVarDecls;
// };

} // namespace krill::minic::ir

#endif