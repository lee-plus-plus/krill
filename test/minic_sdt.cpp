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
using namespace krill::runtime;
using namespace krill::minic;
using namespace krill::ir;

using krill::log::logger;

// label for backpatching
// constexpr Lbl        lbl_continue_base = {.name = "continue_backpatching"};
// constexpr Lbl        lbl_break_base    = {.name = "break_backpatching"};
// constexpr Lbl        lbl_return_base   = {.name = "return_backpatching"};
// constexpr const Lbl *lbl_continue      = &lbl_continue_base;
// constexpr const Lbl *lbl_break         = &lbl_break_base;
// constexpr const Lbl *lbl_return        = &lbl_return_base;

// domain stack, make it easier in sdt
// notice: declarations will be directly appended onto the back of domain
vector<vector<Var *> *> varDomains;
// no backpatching is needed
stack<Lbl *> lbl_continue_stack;
stack<Lbl *> lbl_break_stack;
stack<Lbl *> lbl_return_stack;


Var *find_varible_by_name(const string &varname) {
    for (auto it = varDomains.rbegin(); it != varDomains.rend(); it++) {
        const auto &domain = *it;
        for (const auto &var : domain) {
            if (var->name == varname) { return var; }
        }
    }
    return nullptr;
}

Func *find_function_by_name(const string &funcname) {
    for (auto it = funcDomains.rbegin(); it != funcDomains.rend(); it++) {
        const auto &domain = *it;
        for (const auto &func : domain) {
            if (func->name == funcname) { return func; }
        }
    }
    return nullptr;
}

struct NameCounter {
    AttrDict cnt_;
    string   assign_unique_name(const string &s) {
        int &idx = (!cnt_.Has<int>(s)) ? cnt_.RefN<int>(s) : cnt_.Ref<int>(s);
        return s + "." + to_string(idx++);
    }
};
NameCounter counter;


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
            constVal = stol(lval, nullptr, 10); // decimal
            break;
        case syntax::HEXNUM:
            constVal = stol(lval, nullptr, 16); // hexadecimal
            break;
        default:
            assert(false);
    }
    return constVal;
}

// type_spec : VOID | INT
TypeSpec parse_basetype(APTnode *node) {
    assert(node->id == syntax::type_spec);

    auto lval = node->child[0].get()->attr.Get<string>("lval");
    switch (lval) {
        case "VOID":
            return TypeSpec::kVoid;
        case "INT":
            return TypeSpec::kInt32;
        default:
            assert(false);
    }
}

// ? : type_spec IDENT ? | type_spec IDENT '[' int_literal ']' ?
Var *parse_var_decls(APTnode *node) {
    assert(node->child[0].get()->id == syntax::type_spec);
    assert(node->child[1].get()->id == syntax::IDENT);

    auto basetype = parse_basetype(node->child[0].get());
    auto varname  = node->child[1].get()->attr.Get<string>("lval");
    auto shape    = vector<int>{};

    if (node->child[2].get()->id == '[' &&
        node->child[3].get()->id == syntax::int_literal) {
        // is array declaration
        int dimSize = parse_int_literal(node->child[3].get());
        shape       = vector<int>{dimSize};
    }
    auto type = TypeDecl{.basetype = basetype, .shape = shape};
    auto var  = assign_new_varible({.name = varname, .type = type});

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
            return vector<Var *>{parse_var_decls(node)};
        default:
            assert(false);
    }
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
    }
}

// in-function declarations
void sdt_local_decls(APTnode *node, Code &code) {
    if (node->id == syntax::local_decls) {
        // local_decls : local_decls local_decl
        for (auto &child : node->child) { sdt_local_decls(child.get(), code); }
        return;
    }
    assert(node->== syntax::local_decl);
    // local_decl : type_spec IDENT ';' |
    //              type_spec IDENT '[' int_literal ']' ';'
    auto var = parse_var_decl(node);
    varDomains.back()->emplace_back(var); // push to current domain
}

void sdt_global_func_decl(APTnode *node) {
    // fun_decl : type_spec FUNCTION_IDENT '(' params ')' compound_stmt |
    //            type_spec FUNCTION_IDENT '(' params ')' ';'
    assert(node->id == syntax::func_decl);

    // process declaration
    // -------------------
    auto get_func_decl = [](const APTnode *const node) -> Func * {
        auto funcname = node->child[1].get()->attr.Get<string>("lval");
        auto params   = parse_params(node->child[3].get());

        auto ret_basetype = parse_basetype(node->child[0].get());
        auto ret_type     = TypeDecl{.basetype = ret_basetype, .shape = {}};
        auto ret = assign_new_varible({.name = ".retval", .type = ret_type});
        auto returns = (ret_basetype != TypeSpec::kVoid) ? vector<Var *>{ret}
                                                         : vector<Var *>{};
        func = assign_new_function(
            {.name = funcname, .params = params, .returns = returns});

        // check re-declaration, if there is, use previous
        auto prevDecl = find_function_by_name(funcname);
        if (prevDecl != nullptr) {
            if (predDecl->getTypeDecl() != func->getTypeDecl()) {
                logger.error("error: conflict declaration of function {}, "
                             "previous declaration {}",
                             func->type().str(), prevDecl->type().str());
            }
            if (predDecl->code.has_value() != false) {
                logger.error("error: re-definition of function {}",
                             func->type().str());
            }
            func = prevDecl;
        }
        return func;
    };
    Func *func = get_func_decl(node);

    // only declaration, pass
    if (node->child[5].get()->id != syntax::compound_stmt) { return; }

    // process definition
    // ------------------

    for (auto &ret : func->returns) {
        ret->name = counter.assign_unique_name("retval");
    }
    auto lbl_entry =
        assign_new_label({.name = counter.assign_unique_name("entry")});
    auto lbl_return =
        assign_new_label({.name = counter.assign_unique_name("return")});
    auto q_entry  = QuadTuple{Op::kLabel, .args_j = {.addr1 = lbl_entry}};
    auto q_return = QuadTuple{Op::kLabel, .args_j = {.addr1 = lbl_return}};

    auto body_code = Code{};

    // make the return position visiable in the sub
    lbl_return_stack.push(lbl_return);
    // make the definition of params and localVars visiable in the sub
    // allow the newly declared variables be appended onto func->localVars
    varDomains.back().emplace_back(&func->params);
    varDomains.back().emplace_back(&func->localVars);
    // parse function definition codes
    sdt_stmt(node->child[6], body_code);
    // recover
    varDomains.pop_back();
    varDomains.pop_back();
    // recover
    lbl_return_stack.pop();

    // head code: allocate local variables, intialize return value
    auto gen_head_code = [const &func]() -> Code {} {
        auto head_code = Code{}
        for (const auto &var : func->returns) {
            head_code.emplace_back(
                {Op::kAlloca,
                 .args_d = {.var = var, .size = var->type.size()}});
        }
        for (const auto &var : func->localVars) {
            head_code.emplace_back(
                {Op::kAlloca,
                 .args_d = {.var = var, .size = var->type.size()}});
        }
        // set default value for retval
        for (const auto &var : func->returns) {
            head_code.emplace_back(
                {Op::kStore, .args_m = {.var = var_zero, .mem = var}});
        }
    };
    // tail code: only one return point
    auto gen_tail_code = [const &func]() -> Code {} {
        auto tail_code = Code{};
        for (int i = 0; i < func->returns.size(); i++) {
            auto &ret      = func->returns[i];
            auto  var_temp = assign_new_varible({.type = ret->type});
            Appender{tail_code}
                .append({{Op::kLoad, .args_m = {.var = var_temp, .mem = ret}}})
                .append({{Op::kRetPut, .args_j = {.var = var_temp, .idx = i}}});
        }
        Appender{tail_code}.append({{Op::kRet}});
    };
    Code head_code = gen_head_code();
    Code tail_code = gen_tail_code();

    // amend head_code, body_code, tail_code
    // -------------------------------------
    auto funcCode = Code{};

    Appender{funcCode}
        .append(head_code)
        .append({q_entry})
        .append(body_code)
        .append({q_return})
        .append(tail_code);

    func->code.emplace(std::move(funcCode));
}

void sdt_global_var_decl(APTnode *node) {
    // var_decl <- type_spec IDENT ';' | ...
    assert(node->id == syntax::var_decl);

    auto var = parse_var_decl(node);
    varDomains.back()->emplace_back(var); // should be on global doamin
}

void sdt_local_var_decl(APTnode *node) {
    // local_decl : type_spec IDENT ';' |
    //              type_spec IDENT '[' int_literal ']' ';‘
    assert(node->id == syntax::local_decl);

    auto var = parse_var_decl(node);
    varDomains.back()->emplace_back(var); // should be on local doamin
}

void sdt_expr_stmt(APTnode *node, Code &code) {
    
}

// entry
void syntax_directed_translation(shared_ptr<APTnode> &node) {
    varDeclsDomains.emplace_back(&globalVarDecls); // global domain
    sdt_visit(node);                               // core

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
