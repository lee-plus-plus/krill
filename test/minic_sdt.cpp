#include "fmt/format.h"
#include "krill/grammar.h"
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
using namespace krill::runtime;
using namespace krill::minic;
using namespace krill::minic::ir;
using krill::grammar::ProdItem;

using krill::log::logger;

// label for backpatching
constexpr Lbl continue_bp_lbl = {-5};
constexpr Lbl break_bp_lbl    = {-6};
constexpr Lbl return_bp_lbl   = {-7};

Var assign_new_varible() {
    static Var newVarId = 1;
    return newVarId++;
}
Lbl assign_new_label() {
    static Lbl newLblId = 1;
    return newLblId++;
}
int assign_new_name_idx(const string &name) {
    static AttrDict cnt;
    int &idx = (!cnt.Has<int>(name)) ? cnt.RefN<int>(name) : cnt.Ref<int>(name);
    return idx++;
}

// everything storaged here
// only pass these to the next stage
vector<FuncDecl> globalFuncDecls;
vector<VarDecl>  globalVarDecls;

// TODO: transform OOD to OOD (put global states into class)

// domain stack, make it easier in sdt
// not used later
vector<vector<VarDecl> *> varDeclsDomains;
vector<vector<LblDecl> *> lblDeclsDomains;

// for getting var / lbl name, read only
map<Var, VarDecl *>  varDecls;
map<Lbl, LblDecl *>  lblDecls;
map<Lbl, FuncDecl *> funcDecls;


VarDecl &get_varible_by_name(const string &varname) {
    for (int i = varDeclsDomains.size() - 1; i >= 0; i--) {
        auto &domain = *varDeclsDomains[i];
        for (VarDecl &decl : domain) {
            if (decl.varname == varname) { return decl; }
        }
    }
    assert(false);
}

bool has_varible_by_name(const string &varname) {
    for (int i = varDeclsDomains.size() - 1; i >= 0; i--) {
        auto &domain = *varDeclsDomains[i];
        for (VarDecl &decl : domain) {
            if (decl.varname == varname) { return true; }
        }
    }
    return false;
}

FuncDecl &get_function_by_name(const string &funcname) {
    for (FuncDecl &decl : globalFuncDecls) {
        if (decl.funcName == funcname) { return decl; }
    }
    assert(false);
}

bool has_function_by_name(const string &funcname) {
    for (FuncDecl &decl : globalFuncDecls) {
        if (decl.funcName == funcname) { return true; }
    }
    return false;
}

FuncDecl createFuncDecl(const string &          func_name,
                        const vector<TypeDecl> &params_type,
                        const TypeSpec &        ret_basetype) {
    auto f_params = vector<VarDecl>{};
    for (int i = 0; i < params_type.size(); i++) {
        f_params.push_back(
            {.type = params_type[i], .varname = "", .var = var_empty});
    }
    auto f_ret = vector<VarDecl>{};
    if (ret_basetype != TypeSpec::kVoid) {
        auto type       = TypeDecl{.basetype = ret_basetype,
                             .shape    = {}};
        auto f_ret_elem = VarDecl{.domain  = VarDomain::kLocal,
                                  .type    = type,
                                  .varname = ".retval",
                                  .var     = var_empty};
        f_ret           = {f_ret_elem};
    }
    auto f_decl = FuncDecl{
        .funcLbl   = {},
        .params    = f_params,
        .localVars = {},
        .localLbls = {},
        .ret       = f_ret,
        .funcName  = func_name,
        .code      = {},
    };
    return f_decl;
}

bool isFuncDeclTypeEqual(const FuncDecl &fd1, const FuncDecl &fd2) {
    if (fd1.params.size() != fd2.params.size()) { return false; };
    if (fd1.ret.size() != fd2.ret.size()) { return false; }
    for (int i = 0; i < fd1.params.size(); i++) {
        if (fd1.params[i].type != fd2.params[i].type) { return false; };
    }
    return true;
}

QuadTuple gen_allocate_code(const VarDecl &decl, const Op &op) {
    assert(op == Op::kAllocate || op == Op::kGlobal);
    int len = 1;
    for (auto s : decl.type.shape) { len *= s; }
    if (decl.type.shape.size() == 0) { len = 0; }
    return QuadTuple{op, {.var_a = decl.var, .size = decl.type.size()}};
};

string lbl_name(const Lbl &lbl) {
    if (lblDecls.count(lbl) == 0) {
        return string{"@"} + to_string(static_cast<int>(lbl));
    } else {
        return string{"@"} + lblDecls.at(lbl)->lblname;
    }
};
string var_name(const Var &var) {
    if (varDecls.count(var) == 0) {
        return string{"%"} + to_string(static_cast<int>(var));
    } else {
        const auto &var_decl = *varDecls.at(var);
        if (var_decl.domain == VarDomain::kLocal) {
            return string{"%"} + var_decl.varname;
        } else if (var_decl.domain == VarDomain::kGlobal) {
            return string{"@"} + var_decl.varname;
        } else {
            assert(false);
        }
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
        auto &v_lhs    = _1.Ref<Var>("var");
        auto &v_rhs    = _3.Ref<Var>("var");
        auto &code_lhs = _1.Ref<Code>("code");
        auto &code_rhs = _3.Ref<Code>("code");

        auto code_b_lhs = Code{};
        auto code_b_rhs = Code{};
        Var  b_lhs;
        Var  b_rhs;
        if (pidx == 43 || pidx == 50) {
            b_lhs        = assign_new_varible();
            b_rhs        = assign_new_varible();
            QuadTuple q1 = {
                .op   = Op::kNeq,
                .args = {.dest = b_lhs, .src1 = v_lhs, .src2 = var_zero}};
            QuadTuple q2 = {
                .op   = Op::kNeq,
                .args = {.dest = b_rhs, .src1 = v_rhs, .src2 = var_zero}};
            code_b_lhs.push_back(q1);
            code_b_rhs.push_back(q2);
        }

        v_0 = assign_new_varible();
        QuadTuple q;
        if (pidx == 43) { // expr <- expr OR expr
            q = {Op::kOr, {.dest = v_0, .src1 = b_lhs, .src2 = b_rhs}};
        } else if (pidx == 44) { // expr <- expr EQ expr
            q = {Op::kEq, {.dest = v_0, .src1 = v_lhs, .src2 = v_rhs}};
        } else if (pidx == 45) { // expr <- expr NE expr
            q = {Op::kNeq, {.dest = v_0, .src1 = v_lhs, .src2 = v_rhs}};
        } else if (pidx == 46) { // expr <- expr LE expr
            q = {Op::kEq, {.dest = v_0, .src1 = v_lhs, .src2 = v_rhs}};
        } else if (pidx == 47) { // expr <- expr '<' expr
            q = {Op::kLt, {.dest = v_0, .src1 = v_lhs, .src2 = v_rhs}};
        } else if (pidx == 48) { // expr <- expr GE expr
            q = {Op::kEq, {.dest = v_0, .src1 = v_rhs, .src2 = v_lhs}};
        } else if (pidx == 49) { // expr <- expr '>' expr
            q = {Op::kLt, {.dest = v_0, .src1 = v_rhs, .src2 = v_lhs}};
        } else if (pidx == 50) { // expr <- expr AND expr
            q = {Op::kAnd, {.dest = v_0, .src1 = b_lhs, .src2 = b_rhs}};
        } else if (pidx == 51) { // expr <- expr '+' expr
            q = {Op::kAdd, {.dest = v_0, .src1 = v_lhs, .src2 = v_rhs}};
        } else if (pidx == 52) { // expr <- expr '-' expr
            q = {Op::kMinus, {.dest = v_0, .src1 = v_lhs, .src2 = v_rhs}};
        } else if (pidx == 53) { // expr '*' expr
            q = {Op::kMult, {.dest = v_0, .src1 = v_lhs, .src2 = v_rhs}};
        } else if (pidx == 54) { // expr '/' expr
            q = {Op::kDiv, {.dest = v_0, .src1 = v_lhs, .src2 = v_rhs}};
        } else if (pidx == 55) { // expr '%' expr
            q = {Op::kMod, {.dest = v_0, .src1 = v_lhs, .src2 = v_rhs}};
        } else if (pidx == 65) { // expr '&'' expr
            q = {Op::kAnd, {.dest = v_0, .src1 = v_lhs, .src2 = v_rhs}};
        } else if (pidx == 66) { // expr '^' expr
            q = {Op::kXor, {.dest = v_0, .src1 = v_lhs, .src2 = v_rhs}};
        } else if (pidx == 68) { // expr LSHIFT expr
            q = {Op::kLShift, {.dest = v_0, .src1 = v_lhs, .src2 = v_rhs}};
        } else if (pidx == 69) { // expr RSHIFT expr
            q = {Op::kRShift, {.dest = v_0, .src1 = v_lhs, .src2 = v_rhs}};
        } else if (pidx == 70) { // expr '|'' expr
            q = {Op::kOr, {.dest = v_0, .src1 = v_lhs, .src2 = v_rhs}};
        }
        Appender{code_0}
            .append(code_lhs)
            .append(code_rhs)
            .append(code_b_lhs)
            .append(code_b_rhs)
            .append({q});

    } else if (isUnaryOprt) { // expr <- '!' expr | ...
        auto &v_src    = _2.Ref<Var>("var");
        auto &code_src = _2.Ref<Code>("code");

        v_0 = assign_new_varible();

        QuadTuple q;
        if (pidx == 56) { // expr <- '!' expr
            q = {.op   = Op::kNeq,
                 .args = {.dest = v_0, .src1 = v_src, .src2 = var_zero}};
        } else if (pidx == 57) { // expr <- '-' expr
            q = {.op   = Op::kMinus,
                 .args = {.dest = v_0, .src1 = var_zero, .src2 = v_src}};
        } else if (pidx == 58) { // expr <- '+' expr
            q = {.op   = Op::kAdd,
                 .args = {.dest = v_0, .src1 = var_zero, .src2 = v_src}};
        } else if (pidx == 59) { // expr <- '$' expr
            q = {.op = Op::kLoad, .args = {.var_m = v_0, .addr_m = v_src}};
        } else if (pidx == 67) { // expr <- '~' expr
            q = {.op   = Op::kXor,
                 .args = {.dest = v_0, .src1 = var_zero, .src2 = v_src}};
        }
        Appender{code_0}.append(code_src).append({q});

    } else if (pidx == 60) { // expr <- '(' expr ')'
        v_0    = _2.Ref<Var>("var");
        code_0 = _2.Ref<Code>("code");

    } else if (pidx == 61) { // expr <- IDENT
        auto varname = _1.Ref<string>("lval");

        VarDecl &decl = get_varible_by_name(varname);
        assert(decl.type.basetype != TypeSpec::kVoid);
        assert(decl.type.shape.size() == 0);
        auto v_m = decl.var;

        v_0    = assign_new_varible();
        auto q = QuadTuple{Op::kLoad, {.var_m = v_0, .addr_m = v_m}};
        Appender{code_0}.append({q});

    } else if (pidx == 62) { // expr <- IDENT '[' expr ']'
        auto  varname  = _1.Ref<string>("lval");
        auto  v_idx    = _3.Ref<Var>("var");
        auto &code_idx = _3.Ref<Code>("code");

        VarDecl &decl = get_varible_by_name(varname);
        assert(decl.type.basetype != TypeSpec::kVoid);
        assert(decl.type.shape.size() == 1);
        auto v_m = decl.var;

        Var  v_width     = assign_new_varible();
        Var  v_offset    = assign_new_varible();
        Var  v_addr      = assign_new_varible();
        Code code_offset = {
            {Op::kAssign, {.var = v_width, .cval = 4}},
            {Op::kMult, {.dest = v_offset, .src1 = v_idx, .src2 = v_width}},
            {Op::kAdd, {.dest = v_addr, .src1 = v_m, .src2 = v_offset}},
        };

        v_0    = assign_new_varible();
        auto q = QuadTuple{Op::kLoad, {.var_m = v_0, .addr_m = v_addr}};
        Appender{code_0}.append(code_idx).append(code_offset).append({q});

    } else if (pidx == 63) { // expr <- IDENT '(' args_ ')'
        auto &args = _3.Ref<vector<Var>>("args");

        auto  funcname = _1.Ref<string>("lval");
        auto &decl     = get_function_by_name(funcname);
        assert(decl.params.size() == args.size());
        assert(decl.ret.size() == 1);
        auto lbl_f = decl.funcLbl.lbl;

        Appender{code_0}.append(_3.Ref<Code>("code"));

        v_0 = assign_new_varible();
        // for (auto arg : args) {
        for (int i = 0; i < args.size(); i++) {
            code_0.push_back(
                {.op = Op::kParamPut, .args = {.var_r = args[i], .argc = i}});
        }
        Appender{code_0}
            .append({{Op::kCall,
                      {.func = lbl_f, .argc = static_cast<int>(args.size())}}})
            .append({{Op::kRetGet, {.var_r = v_0, .argc = 0}}});


    } else if (pidx == 64) { // expr <- int_literal
        auto &cval = _1.Ref<int32_t>("cval");
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

    if (pidx == 27) { // expr_stmt <- IDENT '=' expr ';'
        auto &code_expr = _3.Ref<Code>("code");
        auto  varname   = _1.Ref<string>("lval");
        auto  v_expr    = _3.Ref<Var>("var");

        VarDecl &decl = get_varible_by_name(varname);
        assert(decl.type.basetype != TypeSpec::kVoid);
        assert(decl.type.shape.size() == 0);
        auto v_m = decl.var;

        auto q = QuadTuple{Op::kStore, {.var_m = v_expr, .addr_m = v_m}};
        Appender{code}.append(code_expr).append({q});

    } else if (pidx == 28) { // expr_stmt <- IDENT '[' expr ']' '=' expr ';
        auto  varname   = _1.Ref<string>("lval");
        auto  v_idx     = _3.Ref<Var>("var");
        auto  v_expr    = _6.Ref<Var>("var");
        auto &code_idx  = _3.Ref<Code>("code");
        auto &code_expr = _6.Ref<Code>("code");

        VarDecl &decl = get_varible_by_name(varname);
        assert(decl.type.basetype != TypeSpec::kVoid);
        assert(decl.type.shape.size() == 1);
        auto v_m = decl.var;

        Var  v_width     = assign_new_varible();
        Var  v_offset    = assign_new_varible();
        Var  v_addr      = assign_new_varible();
        Code code_offset = {
            {Op::kAssign, {.var = v_width, .cval = 4}},
            {Op::kMult, {.dest = v_offset, .src1 = v_idx, .src2 = v_width}},
            {Op::kAdd, {.dest = v_addr, .src1 = v_m, .src2 = v_offset}},
        };

        auto q = QuadTuple{Op::kStore, {.var_m = v_expr, .addr_m = v_addr}};
        Appender{code}
            .append(code_idx)
            .append(code_offset)
            .append(code_expr)
            .append({q});

    } else if (pidx == 29) { // expr_stmt <- '$' expr '=' expr ';'
        auto  v_addr    = _2.Ref<Var>("var");
        auto  v_expr    = _4.Ref<Var>("var");
        auto &code_addr = _2.Ref<Code>("code");
        auto &code_expr = _4.Ref<Code>("code");

        auto q = QuadTuple{Op::kStore, {.var_m = v_expr, .addr_m = v_addr}};
        Appender{code}.append(code_addr).append(code_expr).append({q});

    } else if (pidx == 30) { // expr_stmt <- IDENT '(' args_ ')' ';'
        auto &args = _3.Ref<vector<Var>>("args");

        auto  funcname = _1.Ref<string>("lval");
        auto &decl     = get_function_by_name(funcname);
        assert(decl.params.size() == args.size());
        // assert(decl.ret.type.basetype != TypeSpec::kVoid); // no check
        auto lbl_f = decl.funcLbl.lbl;

        Appender{code}.append(_3.Ref<Code>("code"));

        for (auto arg : args) {
            code.push_back({.op = Op::kParamPut, .args = {.var = arg}});
        }
        code.push_back(
            {Op::kCall,
             {.func = lbl_f, .argc = static_cast<int>(args.size())}});
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
    if (pidx == 0) { _0 = _1; }
    // decl_list <- decl_list decl | decl
    if (pidx == 1 || pidx == 2) {
        auto &text_code = _0.RefN<Code>("text_code");
        auto &data_code = _0.RefN<Code>("data_code");

        if (pidx == 1) {
            auto &text_code_1 = _1.Ref<Code>("text_code");
            auto &text_code_2 = _2.Ref<Code>("text_code");
            Appender{text_code}.append(text_code_1).append(text_code_2);
            auto &data_code_1 = _1.Ref<Code>("data_code");
            auto &data_code_2 = _2.Ref<Code>("data_code");
            Appender{data_code}.append(data_code_1).append(data_code_2);
        } else {
            auto &text_code_1 = _1.Ref<Code>("text_code");
            Appender{text_code}.append(text_code_1);
            auto &data_code_1 = _1.Ref<Code>("data_code");
            Appender{data_code}.append(data_code_1);
        }
    }

    // decl <- var_decl | fun_decl
    if (pidx == 3 || pidx == 4) {
        auto &text_code = _0.RefN<Code>("text_code");
        auto &data_code = _0.RefN<Code>("data_code");

        if (pidx == 3) {
            // push into global varaible domain
            auto &var_decl = _1.Ref<VarDecl>("var_decl");
            assert(!has_varible_by_name(var_decl.varname));
            globalVarDecls.push_back(var_decl);
            auto q    = gen_allocate_code(var_decl, Op::kGlobal);
            data_code = {q};
        } else if (pidx == 4) {
            // already pushed into global function domain
            auto &func_name = _1.Ref<string>("func_name");
            text_code       = get_function_by_name(func_name).code;
        }
    }

    // var_decl <- type_spec IDENT ';' | ...
    if (pidx == 5 || pidx == 6) {
        auto &var_decl = _0.RefN<VarDecl>("var_decl");

        auto varname  = _2.Ref<string>("lval");
        auto var      = assign_new_varible();
        auto basetype = _1.Ref<TypeSpec>("basetype");
        auto shape    = vector<int>{};

        if (pidx == 6) {
            int arraySize = _4.Ref<int32_t>("cval");
            shape         = vector<int>{arraySize};
        }

        auto type =
            TypeDecl{.basetype = basetype, .shape = shape};
        var_decl = VarDecl{.domain  = VarDomain::kGlobal,
                           .type    = type,
                           .varname = varname,
                           .var     = var};
    }

    // type_spec <- VOID | INT
    if (pidx == 7 || pidx == 8) {
        auto &basetype = _0.RefN<TypeSpec>("basetype");

        if (pidx == 7) {
            basetype = TypeSpec::kVoid;
        } else if (pidx == 8) {
            basetype = TypeSpec::kInt32;
        }
    }

    // func_decl <- type_spec FUNCTION_IDENT '(' params ')'
    // compound_stmt ●
    if (pidx == 9) {
        auto &f_name = _2.Ref<string>("lval");

        /* please check sdt_action when pidx == 9 and dot == 5 */

        auto &f = get_function_by_name(f_name);

        // head code: declare retval, params, local vars, and initialize them
        auto head_code = Code{};

        int  entry_idx = assign_new_name_idx("entry");
        auto lbl_entry = LblDecl{.lblname = "entry." + to_string(entry_idx),
                                 .lbl     = assign_new_label()};
        lblDeclsDomains.back()->push_back(lbl_entry);

        for (int i = 0; i < f.params.size(); i++) {
            head_code.push_back(
                {Op::kParam, {.var_r = f.params[i].var, .argc = i}});
        }
        // allocate stack space for retval, params, local parameters
        for (const auto &decl : f.ret) {
            head_code.push_back(gen_allocate_code(decl, Op::kAllocate));
        }
        for (const auto &decl : f.localVars) {
            head_code.push_back(gen_allocate_code(decl, Op::kAllocate));
        }
        // initialize retval
        for (const auto &decl : f.ret) {
            head_code.push_back(
                {Op::kStore, {.var_m = var_zero, .addr_m = decl.var}});
        }


        // body code:
        auto &body_code = _6.Ref<Code>("code");

        // assign return label
        int  return_idx = assign_new_name_idx("return");
        auto lbl_ret    = LblDecl{.lblname = "ret." + to_string(return_idx),
                               .lbl     = assign_new_label()};
        lblDeclsDomains.back()->push_back(lbl_ret);

        // backpatching, eliminate "return"
        for (int i = 0; i < body_code.size(); i++) {
            auto &q = body_code[i];
            if (q.op == Op::kBackPatch && q.args.func == return_bp_lbl) {
                if (q.args.argc != f.ret.size()) {
                    logger.error("syntax error: unmatched return value count "
                                 "in function {}(), expected {}, got {}",
                                 f.funcName, f.ret.size(), q.args.argc);
                    throw std::runtime_error(fmt::format(
                        "syntax error: unmatched return value count "
                        "in function {}(), expected {}, got {}",
                        f.funcName, f.ret.size(), q.args.argc));
                }
                q = {Op::kGoto, {.addr1 = lbl_ret.lbl}};
                for (int j = 1; j <= f.ret.size(); j++) {
                    assert(i - j >= 0);
                    assert(body_code[i - j].op == Op::kStore);
                    body_code[i - 1].args.addr_m = f.ret[f.ret.size() - j].var;
                }
            }
        }

        // tail code: generating the only return block
        auto tail_code = Code{};
        Appender{tail_code}.append({{Op::kLabel, {.addr1 = lbl_ret.lbl}}});
        for (int i = 0; i < f.ret.size(); i++) {
            auto v_ret = assign_new_varible();
            Appender{tail_code}
                .append({{Op::kLoad, {.var_m = v_ret, .addr_m = f.ret[i].var}}})
                .append({{Op::kRetPut, {.var_r = v_ret, .argc = i}}});
        }
        Appender{tail_code}.append({{Op::kRet, {.argc = int(f.ret.size())}}});

        // amend code
        auto q_func_begin = QuadTuple{Op::kFuncBegin, {.func = f.funcLbl.lbl}};
        auto q_func_end   = QuadTuple{Op::kFuncEnd};
        auto q_entry      = QuadTuple{Op::kLabel, {.addr1 = lbl_entry.lbl}};
        Appender{f.code}
            .append({q_func_begin})
            .append(head_code)
            .append({q_entry})
            .append(body_code)
            .append(tail_code)
            .append({q_func_end});

        // pop domain
        varDeclsDomains.pop_back(); // pop local_decls
        varDeclsDomains.pop_back(); // pop params_decls
        lblDeclsDomains.pop_back();
    }

    // func_decl <- type_spec FUNCTION_IDENT '(' params ')' ';' ●
    if (pidx == 10) {
        auto &f_name = _0.RefN<string>("func_name");

        f_name = _2.Ref<string>("lval");
        // add the type-decl of function into global
        // no name information nor variable is assigned
        auto f_params_type  = _4.Ref<vector<TypeDecl>>("params_type");
        auto f_ret_basetype = _1.Ref<TypeSpec>("basetype");

        auto f_decl = createFuncDecl(f_name, f_params_type, f_ret_basetype);
        assert(has_function_by_name(f_name) == false);

        f_decl.funcLbl = {.lblname = f_decl.funcName,
                          .lbl = assign_new_label()}; // make it Referenceable
        globalFuncDecls.push_back(f_decl);
    }

    // FUNCTION_IDENT <- IDENT
    if (pidx == 11) { _0 = _1; }

    // params <- param_list | VOID
    if (pidx == 12 || pidx == 13) {
        auto &params_name = _0.RefN<vector<string>>("params_name");
        auto &params_type = _0.RefN<vector<TypeDecl>>("params_type");

        if (pidx == 12) {
            params_name = _1.Ref<vector<string>>("params_name");
            params_type = _1.Ref<vector<TypeDecl>>("params_type");
        }
    }

    // params_list <- param_list ',' param | param
    if (pidx == 14 || pidx == 15) {
        auto &params_name = _0.RefN<vector<string>>("params_name");
        auto &params_type = _0.RefN<vector<TypeDecl>>("params_type");

        if (pidx == 14) {
            Appender{params_name}
                .append(_1.Ref<vector<string>>("params_name"))
                .append({_3.Ref<string>("param_name")});
            Appender{params_type}
                .append(_1.Ref<vector<TypeDecl>>("params_type"))
                .append({_3.Ref<TypeDecl>("param_type")});
        } else if (pidx == 15) {
            Appender{params_name}.append({_1.Ref<string>("param_name")});
            Appender{params_type}.append({_1.Ref<TypeDecl>("param_type")});
        }
    }

    // param <- type_spec IDENT | type_spec IDENT '[' int_literal ']'
    if (pidx == 16 || pidx == 17) {
        auto &param_name = _0.RefN<string>("param_name");
        auto &param_type = _0.RefN<TypeDecl>("param_type");

        param_name = _2.Ref<string>("lval");

        auto basetype = _1.Ref<TypeSpec>("basetype");
        auto shape    = vector<int>{};
        if (pidx == 17) {
            int size = _4.Ref<int32_t>("cval");
            shape    = {size};
        }
        param_type = {.basetype = basetype,
                      .shape    = shape};
    }

    // stmt_list <- stmt_list stmt |
    if (pidx == 18 || pidx == 19) {
        auto &code = _0.RefN<Code>("code");

        if (pidx == 18) {
            auto &code_1 = _1.Ref<Code>("code");
            auto &code_2 = _2.Ref<Code>("code");
            Appender{code}.append(code_1).append(code_2);
        }
    }

    // stmt <- expr_stmt | block_stmt | if_stmt | while_stmt | ...
    if (20 <= pidx && pidx <= 26) {
        auto &code = _0.RefN<Code>("code");

        code = _1.Ref<Code>("code");
    }

    // expr_stmt <- IDENT '=' expr ';' | ...
    if (27 <= pidx && pidx <= 30) {
        auto &code = _0.RefN<Code>("code");
        code       = {};

        sdt_expr_stmt_action(parent, child, pidx);
    }

    // while_stmt <- WHILE_IDENT '(' expr ')' stmt ●
    if (pidx == 31) {
        auto &while_idx = _0.Ref<int>("while_idx");
        auto &cond_lbl  = _0.Ref<LblDecl>("cond_lbl");
        auto &body_lbl  = _0.Ref<LblDecl>("body_lbl");
        auto &end_lbl   = _0.RefN<LblDecl>("end_lbl");
        auto &code      = _0.RefN<Code>("code");

        auto &code_expr = _3.Ref<Code>("code");
        auto &code_stmt = _5.Ref<Code>("code");
        auto  v_expr    = _3.Ref<Var>("var");

        end_lbl = LblDecl{.lblname = "while.end." + to_string(while_idx),
                          .lbl     = assign_new_label()};
        lblDeclsDomains.back()->push_back(end_lbl);

        // backpatching - continue
        for (auto &q : code_stmt) {
            if (q.op == Op::kBackPatch && q.args.func == continue_bp_lbl) {
                q = QuadTuple{Op::kGoto, {.addr1 = cond_lbl.lbl}};
            }
        }
        // backpatching - break
        for (auto &q : code_stmt) {
            if (q.op == Op::kBackPatch && q.args.func == break_bp_lbl) {
                q = QuadTuple{Op::kGoto, {.addr1 = end_lbl.lbl}};
            }
        }

        // amend code
        Appender{code}
            .append({{Op::kLabel, {.addr1 = cond_lbl.lbl}}})
            .append(code_expr)
            .append({{.op   = Op::kBranch,
                      .args = {.var_j = v_expr,
                               .addr1 = body_lbl.lbl,
                               .addr2 = end_lbl.lbl}}})
            .append({{Op::kLabel, {.addr1 = body_lbl.lbl}}})
            .append(code_stmt)
            .append({{.op = Op::kGoto, .args = {.addr1 = cond_lbl.lbl}}})
            .append({{Op::kLabel, {.addr1 = end_lbl.lbl}}});
    }

    // WHILE_IDENT <- WHILE
    if (pidx == 32) {}

    // block_stmt <- '{' stmt_list '}'
    if (pidx == 33) { _0 = _2; }

    // compound_stmt <- '{' local_decls stmt_list '}'
    if (pidx == 34) {
        auto &var_decls = _0.RefN<vector<VarDecl>>("var_decls");
        auto &code      = _0.RefN<Code>("code");

        auto &var_decls_local = _2.Ref<vector<VarDecl>>("var_decls");
        auto &code_stmts      = _3.Ref<Code>("code");
        var_decls             = var_decls_local;
        code                  = code_stmts;
    }

    // local_decls <- local_decls local_decl |
    if (pidx == 35 || pidx == 36) {
        auto &var_decls = _0.RefN<vector<VarDecl>>("var_decls");

        if (pidx == 35) {
            var_decls = _1.Ref<vector<VarDecl>>("var_decls");
            var_decls.push_back(_2.Ref<VarDecl>("var_decl"));
        }
    }

    // local_decl <- type_spec IDENT ';' | type_spec IDENT '[' int_literal ']'
    // ';'
    if (pidx == 37 || pidx == 38) {
        auto &var_decl = _0.RefN<VarDecl>("var_decl");

        auto basetype = _1.Ref<TypeSpec>("basetype");
        auto varname  = _2.Ref<string>("lval");
        auto shape    = vector<int>{};
        auto var      = assign_new_varible();

        if (pidx == 38) {
            int arraySize = _4.Ref<int32_t>("cval");
            shape         = vector<int>{arraySize};
        }

        var_decl = VarDecl{
            .domain = VarDomain::kLocal,
            .type =
                TypeDecl{.basetype = basetype, .shape = shape},
            .varname = varname,
            .var     = var};
        varDeclsDomains.back()->push_back(var_decl);
    }

    // if_stmt -> IF '(' expr ')' stmt | IF '(' expr ')' stmt ELSE stmt
    if (pidx == 39 || pidx == 40) {
        auto &code = _0.RefN<Code>("code");

        auto if_idx = assign_new_name_idx("if");
        if (pidx == 39) {
            auto  v_expr         = _3.Ref<Var>("var");
            auto &code_expr      = _3.Ref<Code>("code");
            auto &code_then_stmt = _5.Ref<Code>("code");

            auto then_lbl = LblDecl{.lblname = "if.then." + to_string(if_idx),
                                    .lbl     = assign_new_label()};
            auto end_lbl  = LblDecl{.lblname = "if.end." + to_string(if_idx),
                                   .lbl     = assign_new_label()};
            lblDeclsDomains.back()->push_back(then_lbl);
            lblDeclsDomains.back()->push_back(end_lbl);

            Appender{code}
                .append(code_expr)
                .append({{Op::kBranch,
                          {.var_j = v_expr,
                           .addr1 = then_lbl.lbl,
                           .addr2 = end_lbl.lbl}}})
                .append({{Op::kLabel, {.addr1 = then_lbl.lbl}}})
                .append(code_then_stmt)
                .append({{Op::kGoto, {.addr1 = end_lbl.lbl}}})
                .append({{Op::kLabel, {.addr1 = end_lbl.lbl}}});

        } else if (pidx == 40) {
            auto  v_expr         = _3.Ref<Var>("var");
            auto &code_expr      = _3.Ref<Code>("code");
            auto &code_then_stmt = _5.Ref<Code>("code");
            auto &code_else_stmt = _7.Ref<Code>("code");

            auto then_lbl = LblDecl{.lblname = "if.then." + to_string(if_idx),
                                    .lbl     = assign_new_label()};
            auto else_lbl = LblDecl{.lblname = "if.else." + to_string(if_idx),
                                    .lbl     = assign_new_label()};
            auto end_lbl  = LblDecl{.lblname = "if.end." + to_string(if_idx),
                                   .lbl     = assign_new_label()};
            lblDeclsDomains.back()->push_back(then_lbl);
            lblDeclsDomains.back()->push_back(else_lbl);
            lblDeclsDomains.back()->push_back(end_lbl);

            Appender{code}
                .append(code_expr)
                .append({{Op::kBranch,
                          {.var_j = v_expr,
                           .addr1 = then_lbl.lbl,
                           .addr2 = end_lbl.lbl}}})
                .append({{Op::kLabel, {.addr1 = then_lbl.lbl}}})
                .append(code_then_stmt)
                .append({{Op::kGoto, {.addr1 = end_lbl.lbl}}})
                .append({{Op::kLabel, {.addr1 = else_lbl.lbl}}})
                .append(code_else_stmt)
                .append({{Op::kGoto, {.addr1 = end_lbl.lbl}}})
                .append({{Op::kLabel, {.addr1 = end_lbl.lbl}}});
        }
    }

    // return_stmt <- RETURN ';' | RETURN expr ';'
    if (pidx == 41 || pidx == 42) {
        auto &code = _0.RefN<Code>("code");

        if (pidx == 41) {
            // wait for backpatching
            auto q =
                QuadTuple{Op::kBackPatch, {.func = return_bp_lbl, .argc = 0}};
            Appender{code}.append({q});
        } else {
            auto  v_expr    = _2.Ref<Var>("var");
            auto &code_expr = _2.Ref<Code>("code");
            // wait for backpatching
            auto q1 =
                QuadTuple{Op::kStore, {.var_m = v_expr, .addr_m = var_empty}};
            auto q2 =
                QuadTuple{Op::kBackPatch, {.func = return_bp_lbl, .argc = 1}};
            Appender{code}.append(code_expr).append({q1, q2});
        }
    }

    // expr <- ...
    if (43 <= pidx && pidx <= 70) {
        auto &var  = _0.RefN<Var>("var");
        auto &code = _0.RefN<Code>("code");

        var  = var_empty;
        code = {};

        sdt_expr_action(parent, child, pidx);
    }

    // int_literal <- DECNUM | HEXNUM
    if (71 <= pidx && pidx <= 72) {
        auto &cval = _0.RefN<int32_t>("cval");

        auto &lval = _1.Ref<string>("lval");
        if (pidx == 71) {
            cval = stol(lval, nullptr, 10);
        } else {
            cval = stol(lval, nullptr, 16);
        }
    }

    // arg_list -> arg_list ',' expr | expr
    if (pidx == 73 || pidx == 74) {
        auto &args = _0.RefN<vector<Var>>("args");
        auto &code = _0.RefN<Code>("code");

        if (pidx == 73) {
            args = _1.Ref<vector<Var>>("args");
            code = _1.Ref<Code>("code");
            args.push_back(_3.Ref<Var>("var"));
            Appender{code}.append(_3.Ref<Code>("code"));
        } else {
            args = {_1.Ref<Var>("var")};
            code = _1.Ref<Code>("code");
        }
    }

    // args_ <- arg_list |
    if (pidx == 75 || pidx == 76) {
        auto &args = _0.RefN<vector<Var>>("args");
        auto &code = _0.RefN<Code>("code");

        if (pidx == 75) {
            args = _1.Ref<vector<Var>>("args");
            code = _1.Ref<Code>("code");
        }
    }

    // continue_stmt <- CONTINUE ';'
    if (pidx == 77) {
        auto &code = _0.RefN<Code>("code");

        auto q = QuadTuple{Op::kBackPatch, {.func = continue_bp_lbl}};
        code.push_back(q);
    }

    // break_stmt <- : BREAK ';'
    if (pidx == 78) {
        auto &code = _0.RefN<Code>("code");

        auto q = QuadTuple{Op::kBackPatch, {.func = break_bp_lbl}};
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

    // func_decl <- type_spec FUNCTION_IDENT '(' params ')' ●
    // compound_stmt
    if (pidx == 9 && dot == 5) {
        auto &f_name = _0.RefN<string>("func_name");

        f_name = _2.Ref<string>("lval");
        // push to global domain
        // notice: the assignment of params, retvar, will be delayed until
        // the definition of function occur (pidx=9, dot=5)
        auto f_params_name  = _4.Ref<vector<string>>("params_name");
        auto f_params_type  = _4.Ref<vector<TypeDecl>>("params_type");
        auto f_ret_basetype = _1.Ref<TypeSpec>("basetype");


        // create definition only by type
        // check re-definition
        auto f_decl0 = createFuncDecl(f_name, f_params_type, f_ret_basetype);
        if (has_function_by_name(f_name)) {
            auto &f_decl_prev = get_function_by_name(f_name);
            assert(isFuncDeclTypeEqual(f_decl_prev, f_decl0));
        } else {
            // make it Referenceable
            f_decl0.funcLbl = {.lblname = f_decl0.funcName,
                               .lbl     = assign_new_label()};
            globalFuncDecls.push_back(f_decl0);
        }

        // make its params, return stmt, Referenceable
        auto &f_decl = get_function_by_name(f_name);
        assert(f_decl.ret.size() <= 1);
        for (auto &retvar : f_decl.ret) {
            retvar.varname = ".retval";
            retvar.var     = assign_new_varible();
        }
        for (int i = 0; i < f_params_name.size(); i++) {
            f_decl.params[i].var     = assign_new_varible();
            f_decl.params[i].varname = f_params_name[i];
        }

        // push domain
        // control its lifetime so that it's only open for its definition
        varDeclsDomains.push_back(&f_decl.params);
        varDeclsDomains.push_back(&f_decl.localVars);
        lblDeclsDomains.push_back(&f_decl.localLbls);
    }

    // while_stmt <- WHILE_IDENT ● '(' expr ')' stmt
    if (pidx == 31 && dot == 1) {
        auto &while_idx = _0.RefN<int>("while_idx");
        auto &cond_lbl  = _0.RefN<LblDecl>("cond_lbl");

        while_idx = assign_new_name_idx("while");
        cond_lbl  = LblDecl{.lblname = "while.cond." + to_string(while_idx),
                           .lbl     = assign_new_label()};
        lblDeclsDomains.back()->push_back(cond_lbl);
    }
    // while_stmt <- WHILE_IDENT ● '(' expr ')' stmt
    if (pidx == 31 && dot == 4) {
        auto &while_idx = _0.Ref<int>("while_idx");
        // auto &cond_lbl  = _0.Ref<LblDecl>("cond_lbl");
        auto &body_lbl = _0.RefN<LblDecl>("body_lbl");

        body_lbl = LblDecl{.lblname = "while.body." + to_string(while_idx),
                           .lbl     = assign_new_label()};
        lblDeclsDomains.back()->push_back(body_lbl);
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

// entry
void syntax_directed_translation(shared_ptr<APTnode> &node) {
    varDeclsDomains.push_back(&globalVarDecls); // global domain
    sdt_visit(node);                            // core

    // collect lbl information
    for (auto &f : globalFuncDecls) {
        auto &l         = f.funcLbl;
        lblDecls[l.lbl] = &l;
        for (auto &l : f.localLbls) { lblDecls[l.lbl] = &l; }
        funcDecls[l.lbl] = &f;
    }
    // collect var information
    for (auto &v : globalVarDecls) { varDecls[v.var] = &v; }
    for (auto &f : globalFuncDecls) {
        logger.debug("func {} ret size {}", f.funcName, f.ret.size());
        for (auto &v : f.ret) { varDecls[v.var] = &v; }
        for (auto &v : f.params) { varDecls[v.var] = &v; }
        for (auto &v : f.localVars) { varDecls[v.var] = &v; }
    }

    logger.info("syntax directed translation complete successfully");
}

string to_string(const TypeDecl &type) {
    stringstream ss;
    if (type.shape.size() == 0) {
        ss << enum_name(type.basetype);
    } else {
        ss << fmt::format("[{} x {}]", fmt::join(type.shape, " x "),
                          enum_name(type.basetype));
    }
    return ss.str();
}

string to_string(const VarDecl &var) {
    stringstream ss;
    ss << to_string(var.type) << " %" << var.var;
    return ss.str();
}

string to_string(const QuadTuple &q) {
    stringstream ss;
    switch (q.op) {
        case Op::kNop:
            ss << fmt::format("{:s}", enum_name(q.op));
            break;
        case Op::kBackPatch:
            ss << fmt::format("{:s}", enum_name(q.op));
            logger.critical("backPatch IR not eliminated");
            break;
        case Op::kAssign:
            ss << fmt::format("{} = {:s} {:d}", var_name(q.args.var),
                              enum_name(q.op), int(q.args.cval));
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
            ss << fmt::format("{} = {:s} {} {}", var_name(q.args.var),
                              enum_name(q.op), var_name(q.args.src1),
                              var_name(q.args.src2));
            break;
        case Op::kAllocate:
        case Op::kGlobal:
            ss << fmt::format("{} = {:s} ({} byte)", var_name(q.args.var_a),
                              enum_name(q.op), q.args.size);
            break;
        case Op::kLoad:
            ss << fmt::format("{} = {:s} ({})", var_name(q.args.var_m),
                              enum_name(q.op), var_name(q.args.addr_m));
            break;
        case Op::kStore:
            ss << fmt::format("({}) = {:s} {}", var_name(q.args.addr_m),
                              enum_name(q.op), var_name(q.args.var_m));
            break;
        case Op::kParamPut:
            ss << fmt::format("{}<{:d}> {} ", enum_name(q.op), int(q.args.argc),
                              var_name(q.args.var_r));
            break;
        case Op::kParam:
            ss << fmt::format("{} = {:s}<{:d}>", var_name(q.args.var_r),
                              enum_name(q.op), int(q.args.argc));
            break;
        case Op::kRetPut:
            ss << fmt::format("{}<{:d}> {} ", enum_name(q.op), int(q.args.argc),
                              var_name(q.args.var_r));
            break;
        case Op::kRetGet:
            ss << fmt::format("{} = {:s}<{:d}>", var_name(q.args.var_r),
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
                              var_name(q.args.var_j), lbl_name(q.args.addr1),
                              lbl_name(q.args.addr2));
            break;
        case Op::kFuncBegin:
            ss << fmt::format("func {} {{", lbl_name(q.args.func));
            break;
        case Op::kFuncEnd:
            ss << fmt::format("}}");
            break;
        default:
            assert(false);
    }
    return ss.str();
}

string to_string(const Code &code) {
    stringstream ss;
    for (const auto &q : code) {
        if (q.op == Op::kLabel || q.op == Op::kFuncEnd ||
            q.op == Op::kFuncBegin) {
            ss << to_string(q) << "\n";
        } else {
            ss << "\t" << to_string(q) << "\n";
        }
    }
    return ss.str();
}

// entry
extern string get_ir_str() {
    stringstream ss;

    // print global variables declaration
    for (const auto &var_decl : globalVarDecls) {
        ss << to_string(gen_allocate_code(var_decl, Op::kGlobal)) << "\n";
    }
    ss << "\n";

    // print global functions declaration
    for (const auto &f : globalFuncDecls) {
        for (const auto &q : f.code) {
            if (q.op == Op::kLabel || q.op == Op::kFuncEnd ||
                q.op == Op::kFuncBegin) {
                ss << to_string(q) << "\n";
            } else {
                ss << "\t" << to_string(q) << "\n";
            }
        }
    }
    return ss.str();
}