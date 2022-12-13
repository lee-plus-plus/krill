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
using namespace std;
using namespace krill::type;
using namespace krill::utils;
using namespace krill::runtime;
using namespace krill::minic;
using namespace krill::grammar;

using krill::log::logger;

struct QuadTuple {
    // using Lbl = uint32_t; // label of address, not memeory address
    // using Varname = uint32_t;
    // using CVal = uint32_t;
    struct Lbl {
        int data;
        Lbl() = default;
        Lbl(int data) : data(data){};
             operator int() const { return data; };
        Lbl &operator++(int) {
            data++;
            return *this;
        };
        bool operator<(const Lbl &l) const { return data < l.data; };
        bool operator==(const Lbl &l) const { return data == l.data; };
    };
    struct Var {
        int data;
        Var() = default;
        Var(int data) : data(data){};
             operator int() const { return data; };
        Var &operator++(int) {
            data++;
            return *this;
        };
        bool operator<(const Var &v) const { return data < v.data; };
        bool operator==(const Var &v) const { return data == v.data; };
    };
    struct CVal {
        int data;
        CVal() = default;
        CVal(int data) : data(data){};
        operator int() const { return data; };
    };
    enum class Op {
        kAssign, /* (var, val) */
        kCopy,   /* (var, src1) */
        kAdd,
        kMinus,
        kMult,
        kDiv,
        kMod, /* (dest, src1, src2) */
        kAnd,
        kOr,
        kXor,
        kNor, /* (dest, src1, src2) */
        kLShift,
        kRShift, /* (dest, src1, src2) */
        kEq,
        kNeq,
        kLeq,
        kLt,      /* (dest, src1, src2) */
        kAlloate, /* (var_m, offset) */
        kLoad,
        kStore, /* (var_m, addr, offset) */
        kPush,
        kPop,    /* (var) */
        kCall,   /* (var_j) */
        kGoto,   /* (var_j) */
        kBranch, /* (var_j, addr1, addr2) */
        kRet,    /* (var) */
        kHalt,   /* () */
    };
    Op op;
    union Data {
        struct {
            Var  var;
            CVal cval;
        }; // assign
        struct {
            Var dest;
            Var src1;
            Var src2;
        }; // calculate
        struct {
            Var  var_m;
            Var  addr;
            CVal offset;
        }; // mem load/store
        struct {
            Var var_j;
            Lbl addr1;
            Lbl addr2;
        }; // jump, branch
    } args;
};

using Op       = QuadTuple::Op;
using Lbl      = QuadTuple::Lbl;
using Var      = QuadTuple::Var;
using CVal     = QuadTuple::CVal;
using Lbls     = set<Lbl>;
using LblTable = map<Lbl, int>;
using Code     = vector<QuadTuple>;

enum class TypeSpec {
    kVoid,
    kInt,
};

struct VarDecl {
    TypeSpec    type;
    vector<int> shape; // int -> {}, int[10] -> {10}, int[10][20] -> {10, 20}
};

struct FuncDecl {
    vector<string>  paramsName;
    vector<VarDecl> paramsDecl;
    vector<Var>     paramsVar; // for creating varnameTbl (domain)
    VarDecl         ret;
    Code            code;
};

const Var v_zero    = 0;
const Lbl lbl_empty = -1;

Var newVarId = 1;
Lbl newLblId = 1;

Var assign_new_varible() { return newVarId++; }
Lbl assign_new_label() { return newLblId++; }

using FuncTbl    = map<Var, FuncDecl>; // global func decls
using VarTbl     = map<Var, VarDecl>;
using VarnameTbl = map<string, Var>;

map<Lbl, int>      lineNoTbl;
FuncTbl            funcTbl;
VarTbl             varTbl;
vector<VarnameTbl> varnameTblStack; // stack: {global, local1, local2, ...}

void set_label_with_lineNO(Lbl lbl, int lineNO) { lineNoTbl[lbl] = lineNO; }
int  get_label_with_lineNO(Lbl lbl) { return lineNoTbl.at(lbl); }
void set_label_with_offset(Lbl lbl, int offset) { lineNoTbl.at(lbl) += offset; }
void regesiter_varname_in_domain(const string &varname, const Var &var) {
    varnameTblStack.back()[varname] = var;
}

Var get_varible_by_name(const string &varname) {
    for (int i = varnameTblStack.size() - 1; i >= 0; i--) {
        const auto &varnameTbl = varnameTblStack[i];
        if (varnameTbl.count(varname) != 0) { return varnameTbl.at(varname); }
    }
    assert(false);
}

Var get_varible_by_name_safe(const string &varname) {
    for (int i = varnameTblStack.size() - 1; i >= 0; i--) {
        const auto &varnameTbl = varnameTblStack[i];
        if (varnameTbl.count(varname) != 0) { return varnameTbl.at(varname); }
    }
    return assign_new_varible();
}


template <typename T> class Appender {
  public:
    T &v_;
    Appender(T &v) : v_(v) {}
    inline Appender &append(const T &v) {
        static_assert(is_vector<T> || is_set<T>, "not support yet");

        if constexpr (is_vector<T>) {
            v_.insert(v_.end(), v.begin(), v.end());
        } else if constexpr (is_set<T>) {
            v_.insert(v.begin(), v.end());
        }
        return *this;
    }
};

// expr <- expr OR expr | expr EQ expr | ...
void sdt_expr_action(AttrDict *parent, AttrDict *child[], int pidx) {
    assert(43 <= pidx && pidx <= 70);
    auto &_0 = *parent;
    auto &_1 = *child[0];
    auto &_2 = *child[1];
    auto &_3 = *child[2];
    // auto &_4 = *child[3];
    // auto &_5 = *child[4];
    // auto &_6 = *child[5];
    // auto &_7 = *child[6];
    // auto &_8 = *child[7];

    auto &v_0    = _0.Ref<Var>("var");
    auto &code_0 = _0.Ref<Code>("code");

    bool isBinocularOprt = (43 <= pidx && pidx <= 55) ||
                           (65 <= pidx && pidx <= 66) ||
                           (68 <= pidx && pidx <= 70);
    bool isUnaryOprt = (56 <= pidx && pidx <= 59) || (pidx == 67);

    if (isBinocularOprt) { // expr <- expr OR expr | ...
        v_0            = assign_new_varible();
        auto &v_lhs    = _1.Ref<Var>("var");
        auto &v_rhs    = _3.Ref<Var>("var");
        auto &code_lhs = _1.Ref<Code>("code");
        auto &code_rhs = _3.Ref<Code>("code");

        Var b_lhs;
        Var b_rhs;
        if (pidx == 43 || pidx == 50) {
            b_lhs        = assign_new_varible();
            b_rhs        = assign_new_varible();
            QuadTuple q1 = {
                .op   = Op::kNeq,
                .args = {.dest = b_lhs, .src1 = v_lhs, .src2 = v_zero}};
            QuadTuple q2 = {
                .op   = Op::kNeq,
                .args = {.dest = b_rhs, .src1 = v_rhs, .src2 = v_zero}};
            code_0.push_back(q1);
            code_0.push_back(q2);
        }

        QuadTuple q;
        if (pidx == 43) { // expr <- expr OR expr
            q = {.op   = Op::kOr,
                 .args = {.dest = v_0, .src1 = b_lhs, .src2 = b_rhs}};
        } else if (pidx == 44) { // expr <- expr EQ expr
            q = {.op   = Op::kEq,
                 .args = {.dest = v_0, .src1 = v_lhs, .src2 = v_rhs}};
        } else if (pidx == 45) { // expr <- expr NE expr
            q = {.op   = Op::kNeq,
                 .args = {.dest = v_0, .src1 = v_lhs, .src2 = v_rhs}};
        } else if (pidx == 46) { // expr <- expr LE expr
            q = {.op   = Op::kEq,
                 .args = {.dest = v_0, .src1 = v_lhs, .src2 = v_rhs}};
        } else if (pidx == 47) { // expr <- expr '<' expr
            q = {.op   = Op::kLt,
                 .args = {.dest = v_0, .src1 = v_lhs, .src2 = v_rhs}};
        } else if (pidx == 48) { // expr <- expr GE expr
            q = {.op   = Op::kEq,
                 .args = {.dest = v_0, .src1 = v_rhs, .src2 = v_lhs}};
        } else if (pidx == 49) { // expr <- expr '>' expr
            q = {.op   = Op::kLt,
                 .args = {.dest = v_0, .src1 = v_rhs, .src2 = v_lhs}};
        } else if (pidx == 50) { // expr <- expr AND expr
            q = {.op   = Op::kAnd,
                 .args = {.dest = v_0, .src1 = b_lhs, .src2 = b_rhs}};
        } else if (pidx == 51) { // expr <- expr '+' expr
            q = {.op   = Op::kAdd,
                 .args = {.dest = v_0, .src1 = v_lhs, .src2 = v_rhs}};
        } else if (pidx == 52) { // expr <- expr '-' expr
            q = {.op   = Op::kMinus,
                 .args = {.dest = v_0, .src1 = v_lhs, .src2 = v_rhs}};
        } else if (pidx == 53) { // expr '*' expr
            q = {.op   = Op::kMult,
                 .args = {.dest = v_0, .src1 = v_lhs, .src2 = v_rhs}};
        } else if (pidx == 54) { // expr '/' expr
            q = {.op   = Op::kDiv,
                 .args = {.dest = v_0, .src1 = v_lhs, .src2 = v_rhs}};
        } else if (pidx == 55) { // expr '%' expr
            q = {.op   = Op::kMod,
                 .args = {.dest = v_0, .src1 = v_lhs, .src2 = v_rhs}};
        } else if (pidx == 65) { // expr '&'' expr
            q = {.op   = Op::kAnd,
                 .args = {.dest = v_0, .src1 = v_lhs, .src2 = v_rhs}};
        } else if (pidx == 66) { // expr '^' expr
            q = {.op   = Op::kXor,
                 .args = {.dest = v_0, .src1 = v_lhs, .src2 = v_rhs}};
        } else if (pidx == 68) { // expr LSHIFT expr
            q = {.op   = Op::kLShift,
                 .args = {.dest = v_0, .src1 = v_lhs, .src2 = v_rhs}};
        } else if (pidx == 69) { // expr RSHIFT expr
            q = {.op   = Op::kRShift,
                 .args = {.dest = v_0, .src1 = v_lhs, .src2 = v_rhs}};
        } else if (pidx == 66) { // expr '|'' expr
            q = {.op   = Op::kOr,
                 .args = {.dest = v_0, .src1 = v_lhs, .src2 = v_rhs}};
        }
        Appender{code_0}.append(code_lhs).append(code_rhs).append({q});
    } else if (isUnaryOprt) { // expr <- '!' expr | ...
        auto &v_src    = _2.Ref<Var>("var");
        auto &code_src = _2.Ref<Code>("code");

        v_0 = assign_new_varible();

        QuadTuple q;
        if (pidx == 56) { // expr <- '!' expr
            q = {.op   = Op::kNeq,
                 .args = {.dest = v_0, .src1 = v_src, .src2 = v_zero}};
        } else if (pidx == 57) { // expr <- '-' expr
            q = {.op   = Op::kMinus,
                 .args = {.dest = v_0, .src1 = v_zero, .src2 = v_src}};
        } else if (pidx == 58) { // expr <- '+' expr
            q = {.op   = Op::kAdd,
                 .args = {.dest = v_0, .src1 = v_zero, .src2 = v_src}};
        } else if (pidx == 59) { // expr <- '$' expr
            q = {.op   = Op::kLoad,
                 .args = {.var_m = v_0, .addr = v_src, .offset = 0}};
        } else if (pidx == 67) { // expr <- '~' expr
            q = {.op   = Op::kXor,
                 .args = {.dest = v_0, .src1 = v_zero, .src2 = v_src}};
        }
        Appender{code_0}.append(code_src).append({q});
    } else if (pidx == 60) { // expr <- '(' expr ')'
        v_0    = _2.Ref<Var>("var");
        code_0 = _2.Ref<Code>("code");
    } else if (pidx == 61) { // expr <- IDENT
        Var v_src = get_varible_by_name(_1.Ref<string>("lval"));
        v_0       = v_src;
    } else if (pidx == 62) { // expr <- IDENT '[' expr ']'
        Code code_idx    = _3.Ref<Code>("code");
        Var  v_src       = get_varible_by_name(_1.Ref<string>("lval"));
        Var  v_idx       = _3.Ref<Var>("var");
        Var  v_width     = assign_new_varible();
        Var  v_offset    = assign_new_varible();
        Var  v_addr      = assign_new_varible();
        Code code_assign = {
            {Op::kAssign, {.var = v_width, .cval = 4}},
            {Op::kMult, {.dest = v_offset, .src1 = v_idx, .src2 = v_width}},
            {Op::kAdd, {.dest = v_addr, .src1 = v_src, .src2 = v_offset}},
        };
        Appender{code_0}.append(code_idx).append(code_assign);
        v_0 = v_addr;
    } else if (pidx == 63) { // expr <- IDENT '(' args_ ')'
        auto &args   = _3.Ref<vector<Var>>("args");
        auto  v_func = get_varible_by_name(_1.Ref<string>("lval"));
        auto  v_ret  = assign_new_varible();
        for (int i = args.size() - 1; i >= 0; i--) {
            code_0.push_back({.op = Op::kPush, .args = {.var = args[i]}});
        }
        code_0.push_back({.op = Op::kCall, .args = {.var_j = v_func}});
        code_0.push_back({.op = Op::kPop, .args = {.var = v_ret}});

    } else if (pidx == 64) { // expr <- int_literal
        auto &cval = _1.Ref<CVal>("cval");
        v_0        = assign_new_varible();
        code_0.push_back(
            {.op = Op::kAssign, .args = {.var = v_0, .cval = cval}});
    }
}

void sdt_expr_stmt_action(AttrDict *parent, AttrDict *child[], int pidx) {
    assert(27 <= pidx && pidx <= 30);
    auto &_0 = *parent;
    auto &_1 = *child[0];
    auto &_2 = *child[1];
    auto &_3 = *child[2];
    auto &_4 = *child[3];
    // auto &_5 = *child[4];
    auto &_6 = *child[5];
    // auto &_7 = *child[6];
    // auto &_8 = *child[7];

    auto &code = _0.Ref<Code>("code");
    // auto &lbls = _0.Ref<Lbls>("lbls");

    if (pidx == 27) { // expr_stmt <- IDENT '=' expr ';'
        auto &code_exp = _3.Ref<Code>("code");
        auto  v_dest   = get_varible_by_name(_1.Ref<string>("lval"));
        auto &v_src    = _3.Ref<Var>("var");
        auto  q        = QuadTuple{Op::kCopy, {.dest = v_dest, .src1 = v_src}};
        Appender{code}.append(code_exp).append({q});

    } else if (pidx == 28) { // IDENT '[' expr ']' '=' expr ';
        Var   v_dest     = get_varible_by_name(_1.Ref<string>("lval"));
        Var & v_idx      = _3.Ref<Var>("var");
        Var & v_exp      = _6.Ref<Var>("var");
        Var   v_width    = assign_new_varible();
        Var   v_offset   = assign_new_varible();
        Var   v_addr     = assign_new_varible();
        Code &code_idx   = _3.Ref<Code>("code");
        Code &code_exp   = _6.Ref<Code>("code");
        Code  code_array = {
            {.op = Op::kAssign, .args = {.var = v_width, .cval = 4}},
            {.op   = Op::kMult,
             .args = {.dest = v_offset, .src1 = v_idx, .src2 = v_width}},
            {.op   = Op::kAdd,
             .args = {.dest = v_addr, .src1 = v_dest, .src2 = v_offset}},
        };
        auto q = QuadTuple{Op::kStore,
                           {.var_m = v_exp, .addr = v_addr, .offset = 0}};
        Appender{code}
            .append(code_exp)
            .append(code_idx)
            .append(code_array)
            .append({q});

    } else if (pidx == 29) { // expr_stmt <- '$' expr '=' expr ';'
        Code &code_addr = _2.Ref<Code>("code");
        Code &code_exp  = _4.Ref<Code>("code");
        Var & v_addr    = _2.Ref<Var>("var");
        Var & v_exp     = _4.Ref<Var>("var");
        auto  q =
            QuadTuple{.op   = Op::kStore,
                      .args = {.var_m = v_exp, .addr = v_addr, .offset = 0}};
        Appender{code}.append(code_addr).append(code_exp).append({q});

    } else if (pidx == 30) { // expr_stmt <- IDENT '(' args_ ')' ';'
        auto &args   = _3.Ref<vector<Var>>("args");
        auto  v_func = get_varible_by_name(_1.Ref<string>("lval"));
        for (int i = args.size() - 1; i >= 0; i--) {
            code.push_back({.op = Op::kPush, .args = {.var = args[i]}});
        }
        // require re-fill correct label
        code.push_back({.op = Op::kCall, .args = {.var_j = v_func}});
    }
}

void sdt_synthetic_action(AttrDict *parent, AttrDict *child[], int pidx) {
    logger.debug("synthetic action pidx={}", pidx);

    auto &_0 = *parent;
    auto &_1 = *child[0];
    auto &_2 = *child[1];
    auto &_3 = *child[2];
    auto &_4 = *child[3];
    auto &_5 = *child[4];
    auto &_6 = *child[5];
    auto &_7 = *child[6];
    // auto &_8 = *child[7];

    // program <- decl_list
    if (pidx == 0) {}
    // decl_list <- decl_list decl | decl
    if (pidx == 1 || pidx == 2) {}

    // decl <- var_decl | fun_decl
    if (pidx == 3 || pidx == 4) {

        if (pidx == 3) {
            // push into global varaible domain
            auto &varname = _2.Ref<string>("varname");
            auto &var     = _2.Ref<Var>("var");
            regesiter_varname_in_domain(varname, var);
        } else if (pidx == 4) {
            // already pushed into global function domain
            // pass
        }
    }

    // var_decl <- type_spec IDENT ';' | ...
    if (pidx == 5 || pidx == 6) {
        auto &varname = _0.RefN<string>("varname");
        auto &var     = _0.RefN<Var>("var");

        varname    = _2.Ref<string>("lval");
        var        = assign_new_varible();
        auto type  = _1.Ref<TypeSpec>("type");
        auto shape = vector<int>{};

        if (pidx == 6) {
            int size = _4.Ref<CVal>("cval");
            shape    = vector<int>{size};
        }
        varTbl[var] = VarDecl{.type = type, .shape = shape};
    }

    // type_spec <- VOID | INT
    if (pidx == 7 || pidx == 8) {
        auto &type = _0.RefN<TypeSpec>("type");

        if (pidx == 7) {
            type = TypeSpec::kVoid;
        } else if (pidx == 8) {
            type = TypeSpec::kInt;
        }
    }

    // func_decl <- type_spec FUNCTION_IDENT '(' params ')' compound_stmt ●
    if (pidx == 9) {
        // auto &f_name = _0.Ref<string>("func_name");
        auto &f_var = _0.Ref<Var>("func_var");

        /* please check sdt_action when pidx == 9 and dot == 5 */
        
        auto &f = funcTbl.at(f_var);

        // amend code
        auto  body_code = _6.Ref<Code>("code");
        auto  head_code = Code{};
        for (int i = 0; i < f.paramsVar.size(); i++) {
            auto var = f.paramsVar[i];
            auto q = QuadTuple{Op::kPop, {.var = var}};
            head_code.push_back(q);
        }
        Appender{f.code}.append(head_code).append(body_code);

        // offset labels
        auto  body_lbls = _6.Ref<Lbls>("lbls");
        for (auto lbl : body_lbls) {
            set_label_with_offset(lbl, head_code.size());
        }

        // pop function domain
        varnameTblStack.pop_back();
    }

    // func_decl <- type_spec FUNCTION_IDENT '(' params ')' ';' ●
    if (pidx == 10) {
        auto &f_name = _0.RefN<string>("func_name");
        auto &f_var  = _0.RefN<Var>("func_var");

        // push to global domain
        auto f_params_name = _4.Ref<vector<string>>("params_name");
        auto f_params_decl = _4.Ref<vector<VarDecl>>("params_decl");
        auto f_ret_type    = _1.Ref<TypeSpec>("type");
        auto f_ret         = VarDecl{.type = f_ret_type, .shape = {}};
        auto f_code        = Code{};

        auto f_decl = FuncDecl{
            .paramsName = f_params_name,
            .paramsDecl = f_params_decl,
            .ret        = f_ret,
            .code       = f_code,
        };

        // TODO: add function re-definition check later
        f_name         = _2.Ref<string>("lval");
        f_var          = get_varible_by_name_safe(f_name);
        funcTbl[f_var] = f_decl;
        regesiter_varname_in_domain(f_name, f_var);
    }

    // FUNCTION_IDENT <- IDENT
    if (pidx == 11) { _0 = _1; }

    // params <- param_list | VOID
    if (pidx == 12 || pidx == 13) {
        auto &params_name = _0.RefN<vector<string>>("params_name");
        auto &params_decl = _0.RefN<vector<VarDecl>>("params_decl");

        if (pidx == 12) {
            params_name = _1.Ref<vector<string>>("params_name");
            params_decl = _1.Ref<vector<VarDecl>>("params_decl");
        }
    }

    // params_list <- param_list ',' param | param
    if (pidx == 14 || pidx == 15) {
        auto &params_name = _0.RefN<vector<string>>("params_name");
        auto &params_decl = _0.RefN<vector<VarDecl>>("params_decl");

        if (pidx == 14) {
            Appender{params_name}
                .append(_1.Ref<vector<string>>("params_name"))
                .append({_3.Ref<string>("param_name")});
            Appender{params_decl}
                .append(_1.Ref<vector<VarDecl>>("params_decl"))
                .append({_3.Ref<VarDecl>("param_decl")});
        } else if (pidx == 15) {
            Appender{params_name}.append({_1.Ref<string>("param_name")});
            Appender{params_decl}.append({_1.Ref<VarDecl>("param_decl")});
        }
    }

    // param <- type_spec IDENT | type_spec IDENT '[' int_literal ']'
    if (pidx == 16 || pidx == 17) {
        auto &param_name = _0.RefN<string>("param_name");
        auto &param_decl = _0.RefN<VarDecl>("param_decl");

        param_name = _2.Ref<string>("lval");

        auto type  = _1.Ref<TypeSpec>("type");
        auto shape = vector<int>{};
        if (pidx == 17) {
            int size = _4.Ref<CVal>("cval");
            shape    = {size};
        }
        param_decl = {.type = type, .shape = shape};
    }

    // stmt_list <- stmt_list stmt |
    if (pidx == 18 || pidx == 19) {
        auto &code = _0.RefN<Code>("code");
        auto &lbls = _0.RefN<Lbls>("lbls"); // labels assigned with lineNo
        auto &continue_lbls = _0.RefN<Lbls>("continue_lbls"); // from "continue"
        auto &break_lbls    = _0.RefN<Lbls>("break_lbls");    // from "break"

        if (pidx == 18) {
            auto &code_1 = _1.Ref<Code>("code");
            auto &code_2 = _2.Ref<Code>("code");
            Appender{code}.append(code_1).append(code_2);

            auto &lbls_1 = _1.Ref<Lbls>("lbls");
            auto &lbls_2 = _2.Ref<Lbls>("lbls");
            Appender{lbls}.append(lbls_1).append(lbls_2);

            auto &continue_lbls1 = _1.Ref<Lbls>("continue_lbls");
            auto &continue_lbls2 = _2.Ref<Lbls>("continue_lbls");
            Appender{continue_lbls}
                .append(continue_lbls1)
                .append(continue_lbls2);

            auto &break_lbls1 = _1.Ref<Lbls>("break_lbls");
            auto &break_lbls2 = _2.Ref<Lbls>("break_lbls");
            Appender{break_lbls}.append(break_lbls1).append(break_lbls2);
        }
    }

    // stmt <- expr_stmt | block_stmt | if_stmt | while_stmt | ...
    if (20 <= pidx && pidx <= 26) {
        auto &code = _0.RefN<Code>("code");
        auto &lbls = _0.RefN<Lbls>("lbls"); // labels assigned with lineNo
        auto &continue_lbls = _0.RefN<Lbls>("continue_lbls"); // from "continue"
        auto &break_lbls    = _0.RefN<Lbls>("break_lbls");    // from "break"

        code = _1.Ref<Code>("code");
        if (_1.Has<Lbls>("lbls")) { lbls = _1.Ref<Lbls>("lbls"); }
        if (_1.Has<Lbls>("continue_lbls")) {
            continue_lbls = _1.Ref<Lbls>("continue_lbls");
        }
        if (_1.Has<Lbls>("break_lbls")) {
            break_lbls = _1.Ref<Lbls>("break_lbls");
        }
    }

    // expr_stmt <- IDENT '=' expr ';' | ...
    if (27 <= pidx && pidx <= 30) {
        auto &code = _0.RefN<Code>("code");
        auto &lbls = _0.RefN<Lbls>("lbls"); // labels assigned with lineNo
        code       = {};
        lbls       = {};

        sdt_expr_stmt_action(parent, child, pidx);
    }

    // while_stmt <- WHILE_IDENT '(' expr ')' stmt
    if (pidx == 31) {
        auto &code = _0.RefN<Code>("code");
        auto &lbls = _0.RefN<Lbls>("lbls"); // labels assigned with lineNo

        auto code_expr = _3.Ref<Code>("code");
        auto code_stmt = _5.Ref<Code>("code");
        auto v_cond    = _3.Ref<Var>("var");

        // amend code
        auto cond_lbl         = assign_new_label();
        auto begin_lbl        = assign_new_label();
        auto end_lbl          = assign_new_label();
        int  cond_lbl_lineNO  = 0;
        int  begin_lbl_lineNO = code_expr.size() + 1;
        int  end_lbl_lineNO   = code_expr.size() + code_stmt.size() + 2;
        set_label_with_lineNO(cond_lbl, cond_lbl_lineNO);
        set_label_with_lineNO(begin_lbl, begin_lbl_lineNO);
        set_label_with_lineNO(end_lbl, end_lbl_lineNO);

        auto q_cond = QuadTuple{
            .op   = Op::kBranch,
            .args = {.var_j = v_cond, .addr1 = begin_lbl, .addr2 = end_lbl}};
        auto q_back = QuadTuple{.op = Op::kGoto, .args = {.addr1 = cond_lbl}};

        Appender{code}
            .append(code_expr)
            .append({q_cond})
            .append(code_stmt)
            .append({q_back});

        // offset labels
        auto lbls_expr = _3.Ref<Lbls>("lbls");
        auto lbls_stmt = _5.Ref<Lbls>("lbls");
        for (auto lbl : lbls_expr) { set_label_with_offset(lbl, 0); }
        for (auto lbl : lbls_stmt) {
            set_label_with_offset(lbl, code_expr.size() + 1);
        }
        Appender{lbls}.append(lbls_expr).append(lbls_stmt);

        // backpatching
        auto continue_lbls = _5.Ref<Lbls>("continue_lbls"); // from "continue"
        auto break_lbls    = _5.Ref<Lbls>("break_lbls");    // from "break"
        for (auto lbl : continue_lbls) {
            auto &q = code[get_label_with_lineNO(lbl)];
            assert(q.op == Op::kGoto);
            q = {Op::kGoto, {.addr1 = cond_lbl}};
            lbls.erase(lbl);
        }
        for (auto lbl : break_lbls) {
            auto &q = code[get_label_with_lineNO(lbl)];
            assert(q.op == Op::kGoto);
            q = {Op::kGoto, {.addr1 = end_lbl}};
            lbls.erase(lbl);
        }

    }

    // WHILE_IDENT <- WHILE
    if (pidx == 32) {}

    // block_stmt <- '{' stmt_list '}'
    if (pidx == 33) { _0 = _2; }

    // compound_stmt <- '{' local_decls stmt_list '}'
    if (pidx == 34) {
        auto &code       = _0.RefN<Code>("code");
        auto &lbls       = _0.RefN<Lbls>("lbls"); // labels assigned with lineNo
        auto &continue_lbls = _0.RefN<Lbls>("continue_lbls"); // from "continue"
        auto &break_lbls   = _0.RefN<Lbls>("break_lbls");   // from "break"

        // pop local-decls domain
        varnameTblStack.pop_back();

        // amend code
        auto code_stmts = _3.Ref<Code>("code");
        code            = code_stmts;

        // amend labels
        lbls       = _3.Ref<Lbls>("lbls");
        continue_lbls = _3.Ref<Lbls>("continue_lbls");
        break_lbls   = _3.Ref<Lbls>("break_lbls");
    }

    // local_decls <- local_decls local_decl |
    if (pidx == 35 || pidx == 36) {

        if (pidx == 35) {
            auto &varnameTbl    = varnameTblStack.back();
            auto &varname       = _2.Ref<string>("varname");
            auto &var           = _2.Ref<Var>("var");
            varnameTbl[varname] = var;
        } else {
            varnameTblStack.push_back({});
            // auto &varnameTbl = varnameTblStack.back();
        }
    }

    // local_decl <- type_spec IDENT ';' | type_spec IDENT '[' int_literal ']'
    // ';'
    if (pidx == 37 || pidx == 38) {
        auto &varname = _0.RefN<string>("varname");
        auto &var     = _0.RefN<Var>("var");

        varname    = _2.Ref<string>("lval");
        var        = assign_new_varible();
        auto type  = _1.Ref<TypeSpec>("type");
        auto shape = vector<int>{};

        if (pidx == 38) {
            int size = _4.Ref<CVal>("cval");
            shape    = vector<int>{size};
        }
        varTbl[var] = VarDecl{.type = type, .shape = shape};
    }

    // if_stmt -> IF '(' expr ')' stmt | | IF '(' expr ')' stmt ELSE stmt
    if (pidx == 39 || pidx == 40) {
        auto &code = _0.RefN<Code>("code");
        auto &lbls = _0.RefN<Lbls>("lbls"); // labels assigned with lineNo

        // amend code
        auto v_expr         = _3.Ref<Var>("var");
        auto code_expr      = _3.Ref<Code>("code");
        auto code_true_stmt = _5.Ref<Code>("code");

        auto true_lbl  = assign_new_label();
        auto false_lbl = assign_new_label();
        auto end_lbl   = assign_new_label();

        int true_lbl_lineNO  = 1;
        int false_lbl_lineNO = code_expr.size() + code_true_stmt.size() + 1;
        set_label_with_lineNO(true_lbl, true_lbl_lineNO);
        set_label_with_lineNO(false_lbl, false_lbl_lineNO);

        auto q =
            QuadTuple{Op::kBranch,
                      {.var_j = v_expr, .addr1 = true_lbl, .addr2 = end_lbl}};
        Appender{code}.append(code_expr).append({q}).append(code_true_stmt);

        if (pidx == 39) {
            int end_lbl_lineNO = false_lbl_lineNO;
            set_label_with_lineNO(end_lbl, end_lbl_lineNO);
            auto q2 = QuadTuple{Op::kGoto, {.addr1 = end_lbl}};
            Appender{code}.append({q2});
        } else if (pidx == 40) {
            auto code_false_stmt = _7.Ref<Code>("code");
            int  end_lbl_lineNO = false_lbl_lineNO + code_false_stmt.size() + 1;
            set_label_with_lineNO(end_lbl, end_lbl_lineNO);
            auto q2 = QuadTuple{Op::kGoto, {.addr1 = end_lbl}};
            Appender{code}.append({q2}).append(code_false_stmt).append({q2});
        }

        // amend labels
        auto lbls_true_stmt  = _5.Ref<Lbls>("lbls");
        auto lbls_false_stmt = _7.Ref<Lbls>("lbls");
        Appender{lbls}.append(lbls_true_stmt).append(lbls_false_stmt);
    }

    // return_stmt <- RETURN ';' | RETURN expr ';'
    if (pidx == 41 || pidx == 42) {
        auto &code = _0.RefN<Code>("code");
        auto &lbls = _0.RefN<Lbls>("lbls");

        code = {};
        lbls = {};

        QuadTuple q;
        if (pidx == 41) {
            q = {.op = Op::kRet};
            Appender{code}.append({q});
        } else {
            auto code_expr = _2.Ref<Code>("code");
            auto lbls_expr = _2.Ref<Lbls>("lbls");
            q = {.op = Op::kRet, .args = {.var_j = _2.Ref<Var>("var")}};
            Appender{code}.append(code_expr).append({q});
        }
    }

    // expr <- ...
    if (43 <= pidx && pidx <= 70) {
        auto &var  = _0.RefN<Var>("var");
        auto &code = _0.RefN<Code>("code");
        auto &lbls = _0.RefN<Lbls>("lbls");

        var  = v_zero;
        code = {};
        lbls = {};

        sdt_expr_action(parent, child, pidx);
    }

    // int_literal <- DECNUM | HEXNUM
    if (71 <= pidx && pidx <= 72) {
        auto &cval = _0.RefN<CVal>("cval");

        auto &lval = _1.Ref<string>("lval");
        if (pidx == 71) {
            cval = stoi(lval, nullptr, 10);
        } else {
            cval = stoi(lval, nullptr, 16);
        }
    }

    // arg_list -> arg_list ',' expr | expr
    if (pidx == 73 || pidx == 74) {
        auto &args = _0.RefN<vector<Var>>("args");

        if (pidx == 73) {
            args = _1.Ref<vector<Var>>("args");
            args.push_back(_3.Ref<Var>("var"));
        } else {
            args.push_back(_1.Ref<Var>("var"));
        }
    }

    // args_ <- arg_list |
    if (pidx == 75 || pidx == 76) {
        auto &args = _0.RefN<vector<Var>>("args");

        if (pidx == 75) { args = _1.Ref<vector<Var>>("args"); }
    }

    // continue <- CONTINUE ';'
    if (pidx == 77) {
        auto &code = _0.RefN<Code>("code");
        auto &lbls = _0.RefN<Lbls>("lbls"); // labels assigned with lineNo
        auto &continue_lbl = _0.RefN<Lbl>("continue_lbl"); // from "continue"

        continue_lbl = assign_new_label();
        set_label_with_lineNO(continue_lbl, 0);
        code = {};
        lbls = {continue_lbl};

        QuadTuple q = {.op = Op::kGoto, .args = {.addr1 = continue_lbl}};
        code.push_back(q);
    }

    // break_stmt <- : BREAK ';'
    if (pidx == 78) {
        auto &code      = _0.RefN<Code>("code");
        auto &lbls      = _0.RefN<Lbls>("lbls"); // labels assigned with lineNo
        auto &break_lbl = _0.RefN<Lbl>("break_lbl"); // from "break"

        break_lbl = assign_new_label();
        set_label_with_lineNO(break_lbl, 0);
        code = {};
        lbls = {break_lbl};

        QuadTuple q = {.op = Op::kGoto, .args = {.addr1 = lbl_empty}};
        code.push_back(q);
    }
}

void sdt_inherited_action(AttrDict *parent, AttrDict *child[], int pidx,
                          int dot) {
    auto &_0 = *parent;
    auto &_1 = *child[0];
    auto &_2 = *child[1];
    // auto &_3 = *child[2];
    auto &_4 = *child[3];
    // auto &_5 = *child[4];
    // auto &_6 = *child[5];
    // auto &_7 = *child[6];
    // auto &_8 = *child[7];

    // func_decl <- type_spec FUNCTION_IDENT '(' params ')' ● compound_stmt
    if (pidx == 9 && dot == 5) {
        auto &f_name = _0.RefN<string>("func_name");
        auto &f_var  = _0.RefN<Var>("func_var");

        // push to global domain
        auto f_params_name = _4.Ref<vector<string>>("params_name");
        auto f_params_decl = _4.Ref<vector<VarDecl>>("params_decl");
        auto f_ret_type    = _1.Ref<TypeSpec>("type");
        auto f_ret         = VarDecl{.type = f_ret_type, .shape = {}};
        auto f_code        = Code{};

        // assign Var for parameters
        auto f_params_var = vector<Var>(f_params_name.size());
        for (auto &v : f_params_var) { v = assign_new_varible(); }

        auto f_decl = FuncDecl{
            .paramsName = f_params_name,
            .paramsDecl = f_params_decl,
            .paramsVar  = f_params_var,
            .ret        = f_ret,
            .code       = f_code,
        };

        // add function re-definition check later
        f_name         = _2.Ref<string>("lval");
        f_var          = get_varible_by_name_safe(f_name);
        funcTbl[f_var] = f_decl;
        regesiter_varname_in_domain(f_name, f_var);

        // push function domain
        VarnameTbl domain;
        for (int i = 0; i < f_params_name.size(); i++) {
            auto &p_name = f_params_name[i];
            auto &p_decl = f_params_decl[i];
            auto &p_var  = f_params_var[i];

            domain[p_name] = p_var;
            varTbl[p_var]  = p_decl;
        }
        varnameTblStack.push_back(domain);
    }
}


void sdt_action(AttrDict *parent, AttrDict *child[], int pidx, int dot) {
    logger.debug("sdt_action pidx={}, dot={} {}", pidx, dot,
                 to_string(ProdItem{.pidx = pidx, .dot = dot}, minicGrammar));

    if (dot != minicGrammar.prods[pidx].right.size()) {
        // inherited attrbute processing
        sdt_inherited_action(parent, child, pidx, dot);
    } else {
        // synthetic attribute processing
        // only for reduction action
        sdt_synthetic_action(parent, child, pidx);
    }
}

void sdt_visit(shared_ptr<APTnode> &node) {
    auto sdt_action_wrapped = [&](APTnode *node, int dot) {
        AttrDict *parent_attr = &node->attr;
        AttrDict *child_attr[node->child.size()];
        for (int i = 0; i < node->child.size(); i++) {
            child_attr[i] = &node->child[i].get()->attr;
        }
        sdt_action(parent_attr, child_attr, node->pidx, dot);
    };

    int i;
    for (i = 0; i < node.get()->child.size(); i++) {
        sdt_action_wrapped(node.get(), i);
        sdt_visit(node.get()->child[i]);
    }
    if (node.get()->pidx != -1) { sdt_action_wrapped(node.get(), i); }
};

void syntax_directed_translation(shared_ptr<APTnode> &node) {
    varnameTblStack.push_back({}); // global domain
    sdt_visit(node);
}

string to_string(const QuadTuple &q) {
    stringstream ss;
    switch (q.op) {
        case Op::kAssign:
            ss << fmt::format("%{:d} = {:s} {:d}", int(q.args.var),
                              enum_name(q.op), int(q.args.cval));
            break;
        case Op::kCopy:
            ss << fmt::format("%{:d} = {:s} %{:d}", int(q.args.var),
                              enum_name(q.op), int(q.args.src1));
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
            ss << fmt::format("%{:d} = {:s} %{:d} %{:d}", int(q.args.var),
                              enum_name(q.op), int(q.args.src1),
                              int(q.args.src2));
            break;
        case Op::kAlloate:
            ss << fmt::format("%{:d} = {:s} {:d}", int(q.args.var_m),
                              enum_name(q.op), int(q.args.offset));
            break;
        case Op::kLoad:
            ss << fmt::format("%{:d} = {:s} {:d} (%{:d})", int(q.args.var_m),
                              enum_name(q.op), int(q.args.offset),
                              int(q.args.addr));
            break;
        case Op::kStore:
            ss << fmt::format("{:d} (%{:d}) = {:s} %{:d}", int(q.args.offset),
                              int(q.args.addr), enum_name(q.op),
                              int(q.args.var_m));
            break;
        case Op::kPush:
        case Op::kPop:
            ss << fmt::format("{:s} %{:d}", enum_name(q.op), int(q.args.var));
            break;
        case Op::kCall:
            ss << fmt::format("{:s} func %{:d}", enum_name(q.op),
                              int(q.args.var));
        case Op::kGoto:
            ss << fmt::format("{:s} @{:d}", enum_name(q.op), int(q.args.addr1));
            break;
        case Op::kBranch:
            ss << fmt::format("{:s} (%{:d}) @{:d} @{:d}", enum_name(q.op),
                              int(q.args.var_j), int(q.args.addr1),
                              int(q.args.addr2));
            break;
        case Op::kRet:
            ss << fmt::format("{:s} %{:d}", enum_name(q.op), int(q.args.var));
            break;
        case Op::kHalt:
            ss << fmt::format("{:s}", enum_name(q.op));
            break;
        default:
            assert(false);
    }
    return ss.str();
}

string get_ir_str() {
    stringstream ss;
    for (auto & [ func_var, func_decl ] : funcTbl) {
        ss << fmt::format("func %{:d}:\n", int(func_var));
        for (const auto &q : func_decl.code) {
            ss << "\t" << to_string(q) << "\n";
        }
    }
    return ss.str();
}

extern void initSyntaxParser() {
    // pass
}