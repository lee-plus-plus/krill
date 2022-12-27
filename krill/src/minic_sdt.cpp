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
using namespace krill::runtime;
using namespace krill::minic;
using namespace krill::ir;
<<<<<<<< HEAD:krill/src/minic_sdt.cpp
using krill::grammar::ProdItem;
========
>>>>>>>> 6fc7e2b (update):test/minic_sdt.cpp

using krill::log::logger;

// label for backpatching
const Lbl *continue_bp_lbl = new Lbl{.name = "continue_backpatching"};
const Lbl *break_bp_lbl    = new Lbl{.name = "break_backpatching"};
const Lbl *return_bp_lbl   = new Lbl{.name = "return_backpatching"};

// domain stack, make it easier in sdt
// notice: declarations will be directly appended onto the back of domain
vector<vector<Var *> *> varDomains;
vector<vector<Lbl *> *> funcDomains;

Var *find_varible_by_name(const string &varname) {
    for (auto it = varDomains.rbegin(); it != varDomains.rend(); it++) {
        const auto &domain = *it;
        for (const auto &var : domain) {
            if (var->name == varname) {
                return var;
            }
        }
    }
    return nullptr;
}

Func *find_function_by_name(const string &funcname) {
    for (auto it = funcDomains.rbegin(); it != funcDomains.rend(); it++) {
        const auto &domain = *it;
        for (const auto &func : domain) {
            if (func->name == funcname) {
                return func;
            }
        }
    }
    return nullptr;
}

struct Counter {
    AttrDict cnt;
    int assign_new_idx(const string &s) {
        int &idx = (!cnt.Has<int>(s)) ? cnt.RefN<int>(s) : cnt.Ref<int>(s);
        return idx++;
    } 
};
Counter counter;



// ---------- syntax directed translation ----------

// int_literal : DECNUM | HEXNUM
int parse_int_literal(APTnode *node) {
    assert(node->id == int_literal);
    assert(node->child.size() == 1);

    auto child = node->child[0].get();
    auto lval  = child->attr.Get<string>("lval");
    int  constVal;

    switch (child->id) {
        case syntax::DECNUM:
            constVal = stol(lval, nullptr, 10);
            break;
        case syntax::HEXNUM:
            constVal = stol(lval, nullptr, 16);
            break;
        default:
            assert(false);
            break;
    }
    return constVal;
}

// type_spec : VOID | INT 
TypeSpec parse_basetype(APTnode *node) {
    // type_spec : VOID | INT 
    assert(node->id == syntax::type_spec);

    auto lval = node->child[0].get()->attr.Get<string>("lval");
    switch (lval) {
        case "VOID":
            return TypeSpec::kVoid;
        case "INT":
            return TypeSpec::kInt32;
        default:
            assert(false);
            break;
    }
}

// ? : type_spec IDENT ? | type_spec IDENT '[' int_literal ']' ?
Var *parse_var_decls(APTnode *node) {
    assert(node->child[0].get()->id == syntax::type_spec);
    assert(node->child[1].get()->id == syntax::IDENT);

    auto basetype = parse_basetype(node->child[0].get());
    auto varname  = node->child[1].get()->attr.Get<string>("lval");
    auto shape    = vector<int>{};

    if (node->child.size() >= 4 &&
        node->child[3].get()->id == syntax::int_literal) {
        int dimSize = parse_int_literal(node->child[3].get());
        shape = vector<int>{dimSize};
    }
    auto type = TypeDecl{.basetype = basetype, .shape = shape};
    auto var = new Var{.name = varname, .type = type};

    return var;
}

// params : param_list | VOID
// param_list : param_list ',' param | param
// param : type_spec IDENT | type_spec IDENT '[' int_literal ']'
vector<Var *> parse_params(APTnode *node) {
    switch (node->id) {
        case syntax::params:
            switch (node->child[0].get()->id) {
                case syntax::param_list:
                    return parse_params(node->child[0]);
                case syntax::VOID:
                    return vector<Var *>{};
                default:
                    assert(false);
            }
            break;
        case syntax::param_list:
            auto result = parse_params(node->child[0].get());
            if (node->child.size() > 1) {
                Appender{result}.append(parse_params(node->child[2].get()));
            }
            return result;
        case syntax::param:
            return vector<Var*>{parse_var_decls(node)};
        default:
            assert(false);
    }
}

Func *parse_func_decl(APTnode *node) {
    
}

// ---------- syntax directed translation ----------

// dispatcher
void sdt_decl(APTnode *node) {
    // decl_list, decl, var_decl, fun_decl
    switch (node->id) {
        case syntax::decl_list: // decl_list : decl_list decl | decl
        case syntax::decl:      // decl : var_decl | fun_decl
            for (auto &child : node->child) { sdt_decl(child.get()); }
        case syntax::var_decl:
            sdt_global_var_decl(node);
        case syntax::fun_decl:
            sdt_global_func_decl(node);
        default:
            assert(false);
    }
}

// dispatcher
void sdt_stmt(APTnode *node, Code &code) {
    // compound_stmt, stmt, stmt, expr_stmt, block_stmt, if_stmt, while_stmt,
    // return_stmt, continue_stmt, break_stmt,
    switch (node->id) {
        case syntax::compound_stmt:
            // compound_stmt :  {' local_decls stmt_list '}'
            sdt_local_decls(node->child[1].get(), code);
            sdt_stmt(node->child[2].get(), code);
            break;
        case syntax::stmt_list:
            // stmt_list : stmt_list stmt |
        case syntax::stmt:
            // stmt : expr_stmt | block_stmt | if_stmt | while_stmt |
            //        return_stmt | continue_stmt | break_stmt
            for (auto &child : node->child) { sdt_stmt(child.get(), code); }
            break;
        case syntax::expr_stmt:
            sdt_expr_stmt(node, code);
            break;
        case syntax::block_stmt:
            // block_stmt : '{' stmt_list '}' | '{' '}'
            if (node->child[1].get()->id == syntax::stmt_list) {
                sdt_stmt(node->child[1].get(), code);
            }
            break;
        case syntax::if_stmt:
            // if_stmt : IF '(' expr ')' stmt | IF '(' expr ')' stmt ELSE stmt
            sdt_if_stmt(node, code);
            break;
        case syntax::while_stmt:
            // while_stmt : WHILE_IDENT '(' expr ')' stmt
            sdt_while_stmt(node, code);
            break;
        case syntax::return_stmt:
            // return_stmt : RETURN ';' | RETURN expr ';'
            sdt_return_stmt(node, code);
            break;
        case syntax::continue_stmt:
            // continue_stmt : CONTINUE ';'
            sdt_continue_stmt(node, code);
            break;
        case syntax::break_stmt:
            // break_stmt : BREAK ';'
            sdt_break_stmt(node, code);
            break;
        default:
            assert(false);
            break;
    }
}

// in-function declarations
void sdt_local_decls(APTnode *node, code) {
    if (node->id == syntax::local_decls) {
        // local_decls : local_decls local_decl
        for (auto &child : node->child) { sdt_local_decls(child.get()); }
        return;
    }
    assert(node-> == syntax::local_decl);
    // local_decl : type_spec IDENT ';' | type_spec IDENT '['
    //              int_literal ']' ';'
    auto var = parse_var_decl(node);
    varDomains.back()->push_back(var);
}

void sdt_global_func_decl(APTnode *node) {
    // fun_decl : type_spec FUNCTION_IDENT '(' params ')' compound_stmt | 
    //            type_spec FUNCTION_IDENT '(' params ')' ';'
    assert(node->id == syntax::func_decl);

    auto funcname     = node->child[1].get()->attr.Get<string>("lval");
    auto params       = parse_params(node->child[3].get());

    auto ret_basetype = parse_basetype(node->child[0].get());
    auto ret_type     = TypeDecl{.basetype = ret_basetype, .shape = {}};
    auto ret          = new Var{.name = ".retval", .type = ret_type};
    auto returns      = (ret_basetype != TypeSpec::kVoid) ? vector<Var *>{ret}
                                                          : vector<Var *>{};
    auto func =
        new Func{.name = funcname, .params = params, .returns = returns};
    auto func_decl = unique_ptr<Func>(func); // in case of forgetting to delete

    // check re-declaration, if there is, use previous
    auto prevDef = find_function_by_name(funcname);
    if (prevDef != nullptr) {
        assert(predDecl->getTypeDecl() == func.get()->getTypeDecl());
        // TODO: check re-definition
        func = prevDef;
    } else {
        functions.push_back(move(func_decl));
        funcDomains.back().push_back(func);
    }

    // only declaration, pass
    if (node->child[5].get()->id != syntax::compound_stmt) {
        return;
    } 
    // make the definition of params, localVars visiable in the sub
    // make the newly declared variables be appended onto func->localVars
    varDomains.back().push_back(&func->params);
    varDomains.back().push_back(&func->localVars);

    // parse function definition codes
    auto body_code = Code{};
    sdt_stmt(node->child[6], body_code);

    varDomains.pop_back();
    varDomains.pop_back();

    // head code
    // ---------
    auto head_code = Code{};

    int  idx_entry = counter.assign_new_idx("entry");
    auto lbl_entry = new Lbl{.name = "entry." + to_string(idx_entry)};

    for (int i = 0; i < func->params.size(); i++) {
        head_code.push_back(
            {Op::kParam, {.var_r = f.params[i], .argc = i}});
    }
    for (const auto &var : f.returns) {
        head_code.push_back(
            {Op::kAllocate, {.var_a = var, .size = var->type.size()}});
    }
    for (const auto &var : f.localVars) {
        head_code.push_back(
            {Op::kAllocate, {.var_a = var, .size = var->type.size()}});
    }
    // set default value for retval
    for (const auto &var : f.returns) {
        head_code.push_back(
            {Op::kStore, {.var_m = var_zero, .addr_m = var}});
    }

    // body code
    // ---------

    // backpatching "return"
    // TODO

    // tail code
    // ---------
    auto tail_code = Code{};

    // only one return point
    Appender{tail_code}.append({{Op::kLabel, {.addr1 = lbl_ret.lbl}}});
    for (int i = 0; i < f.ret.size(); i++) {
        auto v_ret = assign_new_varible();
        Appender{tail_code}
            .append({{Op::kLoad, {.var_m = v_ret, .addr_m = f.ret[i].var}}})
            .append({{Op::kRetPut, {.var_r = v_ret, .argc = i}}});
    }
    Appender{tail_code}.append({{Op::kRet, {.argc = int(f.ret.size())}}});

    // amend head_code, body_code, tail_code
    // -------------------------------------
    auto funcCode = Code{};

    auto q_func_begin = QuadTuple{Op::kFuncBegin, {.func = func}};
    auto q_func_end   = QuadTuple{Op::kFuncEnd};
    auto q_entry      = QuadTuple{Op::kLabel, {.addr1 = lbl_entry}};
    Appender{funcCode}
        .append({q_func_begin})
        .append(head_code)
        .append({q_entry})
        .append(body_code)
        .append(tail_code)
        .append({q_func_end});

    Appender{globalCode}.append(funcCode);
    
}

void sdt_global_var_decl(APTnode *node) {
    // var_decl <- type_spec IDENT ';' | ...
    assert(node->id == syntax::var_decl);

    auto var = parse_var_decl(node);
    variables.push_back(unique_ptr(var));
    globalVarDecls.push_back(var);
}

void sdt_local_var_decl(APTnode *node) {
    // local_decl : type_spec IDENT ';' |
    //              type_spec IDENT '[' int_literal ']' ';‘
    assert(node->id == syntax::var_decl);

    auto var = parse_var_decl(node);
    variables.push_back(unique_ptr(var));
    varDeclsDomains.back()->push_back(var_decl);
}

void sdt_expr_stmt(APTnode *node, Code &code) {
    
}

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

// ---------- to string form intermediate representation ----------
