#ifndef IR_H
#define IR_H
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
using std::make_unique, std::unique_ptr;
using std::vector, std::map, std::string, std::optional;

namespace krill::ir {

struct Var;
struct Lbl;
struct Func;

enum class Op {
    // clang-format off
    kNop, 
    kAssign, /* .args_i (var, cval) */
    kAdd, kSub, kMult, kDiv, kMod,/* .args_e (dest, src1, src2) */
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
    kParamPut, kRetPut, kRetGet, /* .args_f (var, idx) */ 
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


// variable (temporal, local, global)
// only store context-free infomation
enum class TypeSpec { kVoid, kInt32 };

struct Var {
    struct TypeDecl {
        TypeSpec    basetype;
        vector<int> shape;

        int    baseSize() const;
        int    size() const;
        string str() const;
        bool   operator==(const TypeDecl &ts) const;
        bool   operator!=(const TypeDecl &ts) const;
        bool   operator<(const TypeDecl &ts) const;
    };
    struct Info {
        // exclusive
        std::optional<int>    constVal;  // it's a const value
        std::optional<int>    fpOffset;  // it's a offset relatived to $fp
        std::optional<int>    memOffset; // it's a data segment position
        std::optional<string> memName;   // it's a data segment label
        std::optional<string> ptrReg;   // 

        std::optional<string> reg; // assigned register
    };

    string   name;
    TypeDecl type;
    // for optimization use
    Info info;

    string fullname() const;
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
        std::optional<int> spOffset; // local space
        std::optional<vector<string>> regsSaved; // callee saved registers
    };

    string        name;
    vector<Var *> params;
    vector<Var *> returns;
    // assigned after definition occur
    optional<Code> code; // if has no value, require to be LINKed!
    vector<Var *>  localVars;
    // for optimization use
    Info info;

    TypeDecl type() const;
    string   fullname() const;
};

inline Var        var_zero_base = {.name = "zero"};
inline Var *const var_zero      = &var_zero_base;

template <typename T> class PtrPool {
  private:
    vector<unique_ptr<T>> pool_;

  public:
    PtrPool()                    = default;
    PtrPool(PtrPool &&pool)      = default;
    PtrPool(const PtrPool &pool) = delete;

    vector<unique_ptr<T>> &elements() { return pool_; }

    T *assign(const T base) {
        auto ownership = std::make_unique<T>(std::move(base));
        auto ptr       = ownership.get();
        pool_.emplace_back(std::move(ownership));
        return ptr;
    }

    void clear() { pool_.clear(); }
};

struct Ir {
    // for livespan management, element can be redundant
    PtrPool<Var>  variables;
    PtrPool<Lbl>  labels;
    PtrPool<Func> functions;

    // main information
    vector<Var *>  globalVars;
    vector<Func *> globalFuncs;

    Ir()             = default;
    Ir(Ir &&ir)      = default;
    Ir(const Ir &ir) = delete;

    void clear();
    Code code();
};

string var_name(Var *var);
string lbl_name(Lbl *lbl);
string lbl_fullname(Lbl *lbl);
string func_name(Func *func);
string func_fullname(Func *func);

string to_string(const QuadTuple &q);
string to_string(const Code &code);

} // namespace krill::ir

#endif