#include "krill/minic_sdt.h"
#include "fmt/format.h"
#include "krill/utils.h"
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>
using namespace std;
using namespace krill::utils;
using namespace krill::minic;
using namespace krill::ir;

using krill::error::parse_error;
using krill::log::logger;
using std::make_shared, std::shared_ptr;

namespace krill::minic {

Var *SdtParser::assign_new_variable(const Var &base) {
    return ir_.variables.assign(base);
}

Lbl *SdtParser::assign_new_label(const Lbl &base) {
    return ir_.labels.assign(base);
}

Func *SdtParser::assign_new_function(const Func &base) {
    return ir_.functions.assign(base);
}

Var *SdtParser::find_varible_by_name(const string &varname) {
    for (auto it = var_domains_.rbegin(); it != var_domains_.rend(); it++) {
        const auto &domain = **it;
        for (const auto &var : domain) {
            if (var->name == varname) { return var; }
        }
    }
    return nullptr;
}

Func *SdtParser::find_function_by_name(const string &funcname) {
    for (auto it = func_domains_.rbegin(); it != func_domains_.rend(); it++) {
        const auto &domain = **it;
        for (const auto &func : domain) {
            if (func->name == funcname) { return func; }
        }
    }
    return nullptr;
}

// ---------- simple parsing ----------

// int_literal : DECNUM | HEXNUM
int SdtParser::parse_int_literal(APTnode *node) {
    assert(node->id == syntax::int_literal);
    assert(node->child.size() == 1);

    auto child0   = node->child[0].get();
    auto lval     = child0->attr.Get<string>("lval");
    int  constVal = 0;

    switch (child0->id) {
    case syntax::DECNUM:
        constVal = stol(lval, nullptr, 10); // decimal
        break;
    case syntax::HEXNUM:
        // std::stol on windows do not accpet hexical begin with "0x" or "0X"
        if (lval.substr(0, 2) == "0x" || lval.substr(0, 2) == "0X") {
            constVal = stoll(lval.substr(2), nullptr, 16); // hexadecimal
        } else {
            constVal = stoll(lval, nullptr, 16); // hexadecimal
        }
        break;
    default:
        assert(false);
    }
    return constVal;
}

// init_list : init_list ',' int_literal | int_literal
vector<int> SdtParser::parse_init_list(APTnode *node) {
    assert(node->id == syntax::init_list);

    if (node->child.size() == 3) {
        auto result = parse_init_list(node->child[0].get());
        result.emplace_back(parse_int_literal(node->child[2].get()));
        return result;
    } else {
        return {parse_int_literal(node->child[0].get())};
    }
}

// type_spec : VOID_ | INT_
TypeSpec SdtParser::parse_basetype(APTnode *node) {
    assert(node->id == syntax::type_spec);

    auto child = node->child[0].get();
    if (child->id == syntax::VOID_) {
        return TypeSpec::kVoid;
    } else if (child->id == syntax::INT_) {
        return TypeSpec::kInt32;
    } else {
        assert(false);
        return TypeSpec::kVoid;
    }
}

// ? : type_spec IDENT ... | type_spec IDENT '[' int_literal ']' ... |
//     type_spec IDENT '=' int_literal ... |
//     type_spec IDENT '[' int_literal ']' '=' '{' init_list '}' ...
Var *SdtParser::parse_var_decl(APTnode *node) {
    assert(node->child[0].get()->id == syntax::type_spec);
    assert(node->child[1].get()->id == syntax::IDENT);

    auto basetype = parse_basetype(node->child[0].get());
    auto varname  = node->child[1].get()->attr.Get<string>("lval");
    auto shape    = vector<int>{};

    bool isArrayDecl =
        (node->child.size() > 2 && node->child[2].get()->id == '[');
    bool hasInitalVal =
        isArrayDecl
            ? (node->child.size() > 5 && node->child[5].get()->id == '=')
            : (node->child.size() > 2 && node->child[2].get()->id == '=');
    // TODO: check previous declaration

    if (isArrayDecl) {
        // is array declaration
        int dimSize = parse_int_literal(node->child[3].get());
        shape       = vector<int>{dimSize};
    }
    auto type = Var::TypeDecl{.basetype = basetype, .shape = shape};
    auto var  = assign_new_variable({.name = varname, .type = type});

    if (hasInitalVal) {
        if (isArrayDecl) {
            // type_spec IDENT '[' int_literal ']' '=' '{' init_list '}' ...
            auto        node_init = node->child[7].get();
            vector<int> init_list = parse_init_list(node_init);

            if (type.shape[0] != init_list.size()) {
                int row = node_init->attr.Get<int>("row_st");
                int col = node_init->attr.Get<int>("col_st");
                throw parse_error(row, col,
                                  "unmatched size for intializer list");
            }
            var->initVal = {init_list};
        } else {
            // type_spec IDENT '=' int_literal ...
            auto node_init = node->child[3].get();
            int  init_val  = parse_int_literal(node_init);

            var->initVal = {vector<int>{init_val}};
        }
    }

    return var;
} // namespace krill::minic

// params : param_list | VOID_
// param_list : param_list ',' param | param
// param : type_spec IDENT | type_spec IDENT '[' int_literal ']'
vector<Var *> SdtParser::parse_params(APTnode *node) {
    switch (node->id) {
    case syntax::params: {
        switch (node->child[0].get()->id) {
        case syntax::param_list:
            return parse_params(node->child[0].get());
        case syntax::VOID_:
            return vector<Var *>{};
        default:
            assert(false);
        }
        break;
    }
    case syntax::param_list: {
        auto result = parse_params(node->child[0].get());
        if (node->child.size() > 1) {
            Appender{result}.append(parse_params(node->child[2].get()));
        }
        return result;
    }
    case syntax::param: {
        auto var = parse_var_decl(node);

        if (var->initVal.has_value()) {
            int row = node->attr.Get<int>("row_st");
            int col = node->attr.Get<int>("col_st");
            throw parse_error(row, col,
                              "cannot define initial value for parameters");
        }
        return vector<Var *>{var};
    }
    default:
        assert(false);
        return {};
    }
}

// IDENT (use)
Var *SdtParser::parse_ident_as_variable(APTnode *node) {
    assert(node->id == syntax::IDENT);

    auto name_ident = node->attr.Get<string>("lval");
    auto var_ident  = find_varible_by_name(name_ident);

    // check exsistance of declaration
    if (var_ident == nullptr) {
        int row = node->attr.Get<int>("row_st");
        int col = node->attr.Get<int>("col_st");
        throw parse_error(row, col,
                          to_string(fmt::format(
                              "using undeclared variable ‘{}’", name_ident)));
    }
    return var_ident;
}

// IDENT (use)
Func *SdtParser::parse_ident_as_function(APTnode *node) {
    assert(node->id == syntax::IDENT);

    auto name_ident = node->attr.Get<string>("lval");
    auto func_ident = find_function_by_name(name_ident);

    // check exsistance of declaration
    if (func_ident == nullptr) {
        int row = node->attr.Get<int>("row_st");
        int col = node->attr.Get<int>("col_st");
        throw parse_error(row, col,
                          to_string(fmt::format(
                              "using undeclared function ‘{}’", name_ident)));
    }
    return func_ident;
}

// ? <- IDENT '[' expr ']' ...
pair<Var *, Code> SdtParser::parse_array_element(APTnode *node, Var *var_ident,
                                                 Var *var_idx) {
    assert(var_ident != nullptr);
    assert(var_idx != nullptr);

    // check array type
    if (var_ident->type.shape.size() < 1) {
        int    row        = node->attr.Get<int>("row_st");
        int    col        = node->attr.Get<int>("col_st");
        string name_ident = var_ident->type.str();
        throw parse_error(row, col,
                          to_string(fmt::format(
                              "try to access ‘{}’ by subscript", name_ident)));
    }

    auto code_dest = Code{};

    auto width      = var_ident->type.baseSize();
    auto var_width  = assign_new_variable({.type = {TypeSpec::kInt32}});
    auto var_offset = assign_new_variable({.type = {TypeSpec::kInt32}});
    auto var_addr   = assign_new_variable({.type = {TypeSpec::kInt32}});

    Appender{code_dest}.append({
        {.op = Op::kAssign, .args_i = {.var = var_width, .cval = width}},
        {.op     = Op::kMult,
         .args_e = {.dest = var_offset, .src1 = var_idx, .src2 = var_width}},
        {.op     = Op::kAdd,
         .args_e = {.dest = var_addr, .src1 = var_ident, .src2 = var_offset}},
    });

    return {var_addr, code_dest};
}

Code SdtParser::parse_function_call(APTnode *node, Func *func,
                                    const vector<Var *> &var_args) {
    // check params type matching
    auto extract_type = [](Var *var) { return var->type; };
    if (apply_map(func->params, extract_type) !=
        apply_map(var_args, extract_type)) {
        int row = node->attr.Get<int>("row_st");
        int col = node->attr.Get<int>("col_st");
        throw parse_error(
            row, col,
            to_string(fmt::format("input parameters' type are not match to "
                                  "the declaration of function ‘{}’",
                                  func->name)));
    }

    auto code_dest = Code{};
    for (int i = 0; i < var_args.size(); i++) {
        code_dest.emplace_back(QuadTuple{
            .op = Op::kParamPut, .args_f = {.var = var_args[i], .idx = i}});
    }
    code_dest.emplace_back(
        QuadTuple{.op = Op::kCall, .args_f = {.func = func}});

    return code_dest;
}

Code SdtParser::parse_local_init_code(Var *var) {
    if (!var->initVal.has_value()) { return {}; }

    auto code_init = Code{};
    if (var->type.shape.size() > 0) {
        // is array
        vector<int> init_list = var->initVal.value();
        int         width     = var->type.baseSize();

        for (int i = 0; i < init_list.size(); i++) {
            auto var_idx    = assign_new_variable({.type = {TypeSpec::kInt32}});
            auto var_width  = assign_new_variable({.type = {TypeSpec::kInt32}});
            auto var_offset = assign_new_variable({.type = {TypeSpec::kInt32}});
            auto var_addr   = assign_new_variable({.type = {TypeSpec::kInt32}});
            auto var_cval   = assign_new_variable({.type = {TypeSpec::kInt32}});

            Appender{code_init}.append({
                {.op = Op::kAssign, .args_i = {.var = var_idx, .cval = i}},
                {.op     = Op::kAssign,
                 .args_i = {.var = var_width, .cval = width}},
                {.op     = Op::kMult,
                 .args_e = {.dest = var_offset,
                            .src1 = var_idx,
                            .src2 = var_width}},
                {.op     = Op::kAdd,
                 .args_e = {.dest = var_addr, .src1 = var, .src2 = var_offset}},
                {.op     = Op::kAssign,
                 .args_i = {.var = var_cval, .cval = init_list[i]}},
                {.op     = Op::kStore,
                 .args_m = {.var = var_cval, .mem = var_addr}},
            });
        }
    } else {
        // is int
        assert(var->initVal.value().size() == 1);
        int init_val = var->initVal.value()[0];

        auto var_cval = assign_new_variable({.type = {TypeSpec::kInt32}});

        Appender{code_init}.append({
            {.op = Op::kAssign, .args_i = {.var = var_cval, .cval = init_val}},
            {.op = Op::kStore, .args_m = {.var = var_cval, .mem = var}},
        });
    }
    return code_init;
}

// ---------- dispatcher ----------

// dispatcher
void SdtParser::sdt_decl(APTnode *node) {
    // decl_list, decl, var_decl, fun_decl, local_decls
    switch (node->id) {
    case syntax::decl_list:   // decl_list : decl_list decl | decl
    case syntax::decl:        // decl : var_decl | fun_decl
    case syntax::local_decls: // local_decls : local_decls local_decl
        for (auto &child : node->child) { sdt_decl(child.get()); }
        break;
    case syntax::var_decl: // var_decl : ...
        sdt_global_var_decl(node);
        break;
    case syntax::fun_decl: // func_decl : ...
        sdt_global_func_decl(node);
        break;
    case syntax::local_decl:
        sdt_local_var_decl(node);
        break;
    default:
        assert(false);
    }
}

// dispatcher
void SdtParser::sdt_stmt(APTnode *node, Code &code) {
    // compound_stmt, stmt, stmt, expr_stmt, block_stmt, if_stmt, while_stmt,
    // return_stmt, continue_stmt, break_stmt,
    assert(node != nullptr);
    switch (node->id) {
    case syntax::compound_stmt:
        // compound_stmt :  {' local_decls stmt_list '}'
        sdt_decl(node->child[1].get());
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
        // block_stmt : '{' local_decls stmt_list '}' | '{' '}'
        sdt_block_stmt(node, code);
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
    case ';':
        break;
    default:
        assert(false);
    }
}

// ---------- syntax directed translation ----------

// global function declaration
void SdtParser::sdt_global_func_decl(APTnode *node) {
    // fun_decl : type_spec FUNCTION_IDENT '(' params ')' compound_stmt |
    //            type_spec FUNCTION_IDENT '(' params ')' ';'
    assert(node->id == syntax::fun_decl);

    // process declaration
    // -------------------
    auto get_func_decl = [this](APTnode *node) -> Func * {
        // FUNCTION_IDENT : IDENT
        auto funcname =
            node->child[1].get()->child[0].get()->attr.Get<string>("lval");
        auto params = this->parse_params(node->child[3].get());

        auto ret_basetype = parse_basetype(node->child[0].get());
        auto ret_type = Var::TypeDecl{.basetype = ret_basetype, .shape = {}};
        auto ret = assign_new_variable({.name = ".retval", .type = ret_type});
        auto returns = (ret_basetype != TypeSpec::kVoid) ? vector<Var *>{ret}
                                                         : vector<Var *>{};
        auto func = assign_new_function(
            {.name = funcname, .params = params, .returns = returns});

        // check re-declaration, if there is, use previous
        auto prevDecl = find_function_by_name(funcname);
        if (prevDecl != nullptr) {
            if (prevDecl->type() != func->type()) {
                int row = node->attr.Get<int>("row_st");
                int col = node->attr.Get<int>("col_st");
                throw parse_error(
                    row, col,
                    to_string(fmt::format(
                        "conflict declaration of function ‘{}’, "
                        "previous declaration ‘{}’",
                        func_fullname(func), func_fullname(prevDecl))));
            }
            if (prevDecl->code.has_value() != false) {
                int row = node->attr.Get<int>("row_st");
                int col = node->attr.Get<int>("col_st");
                throw parse_error(
                    row, col,
                    to_string(fmt::format("re-definition of function ‘{}’",
                                          func->name)));
            }
            logger.debug("sdt: found previous declaration of function ‘{}’", func->name);

            func = prevDecl;
        } else {
            // fix bug: previous declared function will be added into ir twice
            // add its declaration into domains
            func_domains_.back()->emplace_back(func);
        }
        return func;
    };
    Func *func = get_func_decl(node);

    // only declaration, pass
    if (node->child[5].get()->id != syntax::compound_stmt) { return; }

    // process definition
    // ------------------

    for (auto &ret : func->returns) {
        ret->name = counter_.assign_unique_name("retval");
    }
    auto lbl_init =
        assign_new_label({.name = counter_.assign_unique_name("init")});
    auto lbl_entry =
        assign_new_label({.name = counter_.assign_unique_name("entry")});
    auto lbl_return =
        assign_new_label({.name = counter_.assign_unique_name("return")});
    auto q_init  = QuadTuple{.op = Op::kLabel, .args_j = {.addr1 = lbl_init}};
    auto q_entry = QuadTuple{.op = Op::kLabel, .args_j = {.addr1 = lbl_entry}};
    auto q_return =
        QuadTuple{.op = Op::kLabel, .args_j = {.addr1 = lbl_return}};

    auto init_code = Code{}; // intialize local variables
    auto body_code = Code{};

    // make the return position visiable in the sub
    lbl_return_stack_.emplace(lbl_return);
    var_returns_stack_.emplace(func->returns);
    // make the definition of params and localVars visiable in the sub
    // allow the newly declared variables be appended onto func->localVars

    var_domains_.emplace_back(&func->params);
    var_domains_.emplace_back(&func->localVars);
    initializer_domains_.emplace_back(&init_code);
    // parse function definition codes
    sdt_stmt(node->child[5].get(), body_code); // compound_stmt
    // recover
    initializer_domains_.pop_back();
    var_domains_.pop_back();
    var_domains_.pop_back();
    // recover
    lbl_return_stack_.pop();
    var_returns_stack_.pop();

    // head code: allocate local variables
    auto gen_head_code = [func]() -> Code {
        auto head_code = Code{};
        for (const auto &var : func->returns) {
            head_code.emplace_back(
                QuadTuple{.op     = Op::kAlloca,
                          .args_d = {.var = var, .size = var->type.size()}});
        }
        for (const auto &var : func->localVars) {
            head_code.emplace_back(
                QuadTuple{.op     = Op::kAlloca,
                          .args_d = {.var = var, .size = var->type.size()}});
        }
        // set default value for retval
        // for (const auto &var : func->returns) {
        //     head_code.emplace_back(QuadTuple{
        //         .op = Op::kStore, .args_m = {.var = var_zero, .mem = var}});
        // }
        return head_code;
    };
    // tail code: only one return point
    auto gen_tail_code = [this, func]() -> Code {
        auto tail_code = Code{};
        for (int i = 0; i < func->returns.size(); i++) {
            auto &ret      = func->returns[i];
            auto  var_temp = this->assign_new_variable({.type = ret->type});
            tail_code.emplace_back(QuadTuple{
                .op = Op::kLoad, .args_m = {.var = var_temp, .mem = ret}});
            tail_code.emplace_back(QuadTuple{
                .op = Op::kRetPut, .args_f = {.var = var_temp, .idx = i}});
        }
        Appender{tail_code}.append(
            {{.op = Op::kRet, .args_f = {.func = func}}});
        return tail_code;
    };
    Code head_code = gen_head_code();
    Code tail_code = gen_tail_code();

    // amend head_code, body_code, tail_code
    // -------------------------------------
    auto funcCode = Code{};

    Appender{funcCode}
        .append({q_init})   //  .init:
        .append(head_code)  //      allocate ...
        .append(init_code)  //      initializer ...
        .append({q_entry})  //  .entry:
        .append(body_code)  //      ...
        .append({q_return}) //  .return:
        .append(tail_code); //      store retval ...

    func->code.emplace(std::move(funcCode));
}

// global variable declaration
void SdtParser::sdt_global_var_decl(APTnode *node) {
    // var_decl : type_spec IDENT ';' | ...
    assert(node->id == syntax::var_decl);

    auto var = parse_var_decl(node);

    // add its declaration, intializer code, into domains
    var_domains_.back()->emplace_back(var);

    if (var->initVal.has_value()) {
        // do nothing
        // the backend will deal with it
    }
}

// in-function variable declarations
void SdtParser::sdt_local_var_decl(APTnode *node) {
    // local_decl : type_spec IDENT ';' |
    //              type_spec IDENT '[' int_literal ']' ';‘
    assert(node->id == syntax::local_decl);

    auto var = parse_var_decl(node);

    // add its declaration, intializer code, into domains
    var_domains_.back()->emplace_back(var);

    if (var->initVal.has_value()) {
        auto code_init = parse_local_init_code(var);
        Appender{*initializer_domains_.back()}.append(code_init);
    }
}

// parse argument list
vector<Var *> SdtParser::sdt_args(APTnode *node, Code &code) {
    // args_ : arg_list |
    // arg_list : arg_list ',' expr  | expr
    switch (node->id) {
    case syntax::args_: {
        if (node->child.size() > 0) {
            return sdt_args(node->child[0].get(), code);
        } else {
            return {};
        }
    }
    case syntax::arg_list: {
        auto var_args1 = sdt_args(node->child[0].get(), code);
        if (node->child.size() >= 3) {
            auto var_args2 = sdt_args(node->child[2].get(), code);
            Appender{var_args1}.append(var_args2);
        }
        return var_args1;
    }
    case syntax::expr: {
        auto[var_expr, code_expr] = sdt_expr(node);
        Appender{code}.append(code_expr);
        return vector<Var *>{var_expr};
    }
    default:
        assert(false);
        return {};
    }
}

// parse expr
pair<Var *, Code> SdtParser::sdt_expr(APTnode *node) {
    // expr : expr OR expr | expr EQ expr | ... | '!' expr | ... |
    //      IDENT | IDENT '[' expr ']' | IDENT '(' args_ ')' | int_literal
    assert(node->id == syntax::expr);

    auto is_binocular_expr = [](const APTnode *node) -> bool {
        return (node->child.size() == 3 &&
                node->child[0].get()->id == syntax::expr &&
                node->child[2].get()->id == syntax::expr);
    };
    auto is_unary_expr = [](const APTnode *node) -> bool {
        return (node->child.size() == 2 &&
                node->child[1].get()->id == syntax::expr);
    };
    auto is_bool_oprt = [](const int &id) -> bool {
        return ((id == syntax::OR) || (id == syntax::AND));
    };

    auto child = node->child;
    if (is_binocular_expr(node)) { // 2-operands expression
        // expr : expr OR expr | ...
        auto oprt             = node->child[1].get()->id;
        auto node_lhs         = node->child[0].get();
        auto node_rhs         = node->child[2].get();
        auto[v_lhs, code_lhs] = sdt_expr(node_lhs);
        auto[v_rhs, code_rhs] = sdt_expr(node_rhs);
        auto v_dest           = assign_new_variable({.type = v_lhs->type});
        auto code_dest        = Code{};

        Appender{code_dest}.append(code_lhs).append(code_rhs);

        // type check
        if (v_lhs->type != v_rhs->type) {
            int row = node->attr.Get<int>("row_st");
            int col = node->attr.Get<int>("col_st");
            throw parse_error(row, col, "operands are not in same type");
        }

        // cast int operands into bool
        if (is_bool_oprt(oprt)) {
            if (v_lhs->type.shape.size() != 0) {
                int    row       = node->attr.Get<int>("row_st");
                int    col       = node->attr.Get<int>("col_st");
                string name_type = v_lhs->type.str();
                throw parse_error(row, col,
                                  to_string(fmt::format(
                                      "try to cast {} into bool ", name_type)));
            }
            if (v_rhs->type.shape.size() != 0) {
                int    row       = node->attr.Get<int>("row_st");
                int    col       = node->attr.Get<int>("col_st");
                string name_type = v_rhs->type.str();
                throw parse_error(row, col,
                                  to_string(fmt::format(
                                      "try to cast {} into bool ", name_type)));
            }

            auto v_bool_lhs = assign_new_variable(
                {.type = {.basetype = TypeSpec::kInt32, .shape = {}}});
            auto v_bool_rhs = assign_new_variable(
                {.type = {.basetype = TypeSpec::kInt32, .shape = {}}});
            auto q_cast_lhs = QuadTuple{.op     = Op::kNeq,
                                        .args_e = {.dest = v_bool_lhs,
                                                   .src1 = v_lhs,
                                                   .src2 = var_zero}};
            auto q_cast_rhs = QuadTuple{.op     = Op::kNeq,
                                        .args_e = {.dest = v_bool_rhs,
                                                   .src1 = v_rhs,
                                                   .src2 = var_zero}};

            code_dest.emplace_back(move(q_cast_lhs));
            code_dest.emplace_back(move(q_cast_rhs));
            v_lhs = v_bool_lhs;
            v_rhs = v_bool_rhs;
        }
        // TODO: add automatic operands of types

        QuadTuple q;
        switch (oprt) {
        case syntax::OR:
            q = {.op     = Op::kOr,
                 .args_e = {.dest = v_dest, .src1 = v_lhs, .src2 = v_rhs}};
            break;
        case syntax::EQ:
            q = {.op     = Op::kEq,
                 .args_e = {.dest = v_dest, .src1 = v_lhs, .src2 = v_rhs}};
            break;
        case syntax::NE:
            q = {.op     = Op::kNeq,
                 .args_e = {.dest = v_dest, .src1 = v_lhs, .src2 = v_rhs}};
            break;
        case syntax::LE:
            q = {.op     = Op::kEq,
                 .args_e = {.dest = v_dest, .src1 = v_lhs, .src2 = v_rhs}};
            break;
        case '<':
            q = {.op     = Op::kLt,
                 .args_e = {.dest = v_dest, .src1 = v_lhs, .src2 = v_rhs}};
            break;
        case syntax::GE:
            q = {.op     = Op::kLt,
                 .args_e = {.dest = v_dest, .src1 = v_rhs, .src2 = v_lhs}};
            break;
        case '>':
            q = {.op     = Op::kLt,
                 .args_e = {.dest = v_dest, .src1 = v_rhs, .src2 = v_lhs}};
            break;
        case syntax::AND:
            q = {.op     = Op::kAnd,
                 .args_e = {.dest = v_dest, .src1 = v_lhs, .src2 = v_rhs}};
            break;
        case '+':
            q = {.op     = Op::kAdd,
                 .args_e = {.dest = v_dest, .src1 = v_lhs, .src2 = v_rhs}};
            break;
        case '-':
            q = {.op     = Op::kSub,
                 .args_e = {.dest = v_dest, .src1 = v_lhs, .src2 = v_rhs}};
            break;
        case '*':
            q = {.op     = Op::kMult,
                 .args_e = {.dest = v_dest, .src1 = v_lhs, .src2 = v_rhs}};
            break;
        case '/':
            q = {.op     = Op::kDiv,
                 .args_e = {.dest = v_dest, .src1 = v_lhs, .src2 = v_rhs}};
            break;
        case '%':
            q = {.op     = Op::kMod,
                 .args_e = {.dest = v_dest, .src1 = v_lhs, .src2 = v_rhs}};
            break;
        case '&':
            q = {.op     = Op::kAnd,
                 .args_e = {.dest = v_dest, .src1 = v_lhs, .src2 = v_rhs}};
            break;
        case '^':
            q = {.op     = Op::kXor,
                 .args_e = {.dest = v_dest, .src1 = v_lhs, .src2 = v_rhs}};
            break;
        case syntax::LSHIFT:
            q = {.op     = Op::kLShift,
                 .args_e = {.dest = v_dest, .src1 = v_lhs, .src2 = v_rhs}};
            break;
        case syntax::RSHIFT:
            q = {.op     = Op::kRShift,
                 .args_e = {.dest = v_dest, .src1 = v_lhs, .src2 = v_rhs}};
            break;
        case '|':
            q = {.op     = Op::kOr,
                 .args_e = {.dest = v_dest, .src1 = v_lhs, .src2 = v_rhs}};
            break;
        default:
            assert(false);
        }
        code_dest.emplace_back(move(q));
        return {v_dest, code_dest};
    } else if (is_unary_expr(node)) { // 1-operand expression
        // expr : '!' expr | ...
        auto oprt             = node->child[0].get()->id;
        auto node_src         = node->child[1].get();
        auto[v_src, code_src] = sdt_expr(node_src);
        auto v_dest           = assign_new_variable({.type = v_src->type});
        auto code_dest        = Code{};

        QuadTuple q;
        switch (oprt) {
        case '-':
            q = {.op     = Op::kSub,
                 .args_e = {.dest = v_dest, .src1 = v_src, .src2 = var_zero}};
            break;
        case '+':
            q = {.op     = Op::kAdd,
                 .args_e = {.dest = v_dest, .src1 = v_src, .src2 = var_zero}};
            break;
        case '!':
            q = {.op     = Op::kNeq,
                 .args_e = {.dest = v_dest, .src1 = v_src, .src2 = var_zero}};
            break;
        case '~':
            q = {.op     = Op::kXor,
                 .args_e = {.dest = v_dest, .src1 = v_src, .src2 = var_zero}};
            break;
        case '$': // get memory
            q = {.op = Op::kLoad, .args_m = {.var = v_dest, .mem = v_src}};
            break;
        default:
            assert(false);
        }
        code_dest.emplace_back(move(q));
        return {v_dest, code_dest};
    } else if (child[0].get()->id == '(') { // parenthesis expression
        // expr : '(' expr ')'
        assert(child[1].get()->id == syntax::expr);

        auto node_src         = child[1].get();
        auto[v_src, code_src] = sdt_expr(node_src);
        return {v_src, code_src};
    } else if (child[0].get()->id == syntax::IDENT && // get variable
               child.size() == 1) {
        // expr : IDENT
        Var *v_ident = parse_ident_as_variable(child[0].get());
        Var *v_dest;
        Code code_dest;

        if (v_ident->type.shape.size() == 0) {
            v_dest    = assign_new_variable({.type = v_ident->type});
            code_dest = Code{
                {.op = Op::kLoad, .args_m = {.var = v_dest, .mem = v_ident}}};
        } else {
            // is a pointer
            v_dest    = v_ident;
            code_dest = {};
        }

        return {v_dest, code_dest};
    } else if (child[0].get()->id == syntax::IDENT && // get array element
               child[1].get()->id == '[') {
        // expr : IDENT '[' expr ']'
        assert(child[2].get()->id == syntax::expr);

        auto var_ident          = parse_ident_as_variable(child[0].get());
        auto[var_idx, code_idx] = sdt_expr(child[2].get());
        auto[var_array, code_array] =
            parse_array_element(child[0].get(), var_ident, var_idx);

        auto var_dest = assign_new_variable(
            {.type = Var::TypeDecl{.basetype = var_ident->type.basetype,
                                   .shape    = {}}});
        auto code_dest = Code{};

        Appender{code_dest}
            .append(code_idx)
            .append(code_array)
            .append({{.op     = Op::kLoad,
                      .args_m = {.var = var_dest, .mem = var_array}}});

        return {var_dest, code_dest};
    } else if (child[0].get()->id == syntax::IDENT && // get function return
               child[1].get()->id == '(') {
        // expr : IDENT '(' args_ ')'
        assert(child[2].get()->id == syntax::args_);

        auto func = parse_ident_as_function(child[0].get());
        // check return type
        if (func->returns.size() != 1) {
            int row = node->attr.Get<int>("row_st");
            int col = node->attr.Get<int>("col_st");
            throw parse_error(
                row, col,
                to_string(fmt::format(
                    "return value number of function ‘{}’ do not match",
                    func->name)));
        }

        auto code_args = Code{};
        auto node_args = child[2].get();
        auto var_args  = sdt_args(node_args, code_args);
        auto code_funccall =
            parse_function_call(child[2].get(), func, var_args);

        auto var_ret   = assign_new_variable({.type = func->returns[0]->type});
        auto code_dest = Code{};
        Appender{code_dest}
            .append(code_args)
            .append(code_funccall)
            .append({QuadTuple{.op     = Op::kRetGet,
                               .args_f = {.var = var_ret, .idx = 0}}});

        return {var_ret, code_dest};
    } else if (child[0].get()->id == syntax::int_literal) {
        auto cval = parse_int_literal(child[0].get());
        auto var =
            assign_new_variable({.type = {.basetype = TypeSpec::kInt32}});
        auto code =
            Code{{.op = Op::kAssign, .args_i = {.var = var, .cval = cval}}};
        return {var, code};
    } else {
        logger.error("expr pidx={}", node->pidx);
        assert(false);
        throw runtime_error("sdt_expr error");
    }
}

// parse expr_stmt
void SdtParser::sdt_expr_stmt(APTnode *node, Code &code) {
    // expr_stmt : IDENT '=' expr ';' | IDENT '[' expr ']' '=' expr ';' |
    //             '$' expr '=' expr ';' | IDENT '(' args_ ')' ';'
    assert(node->id == syntax::expr_stmt);

    auto &child = node->child;
    if (child[0].get()->id == syntax::IDENT && // set variable
        child[1].get()->id == '=') {
        // expr_stmt : IDENT '=' expr ';'
        assert(child[2].get()->id == syntax::expr);

        auto var_ident            = parse_ident_as_variable(child[0].get());
        auto[var_expr, code_expr] = sdt_expr(child[2].get());
        Appender{code}.append(code_expr).append(
            {{.op     = Op::kStore,
              .args_m = {.var = var_expr, .mem = var_ident}}});
        return;
    } else if (child[0].get()->id == syntax::IDENT && // set array element
               child[1].get()->id == '[') {
        // expr_stmt : IDENT '[' expr ']' '=' expr ';'
        assert(child[2].get()->id == syntax::expr);
        assert(child[5].get()->id == syntax::expr);

        auto var_ident          = parse_ident_as_variable(child[0].get());
        auto[var_idx, code_idx] = sdt_expr(child[2].get());
        auto[var_array, code_array] =
            parse_array_element(child[0].get(), var_ident, var_idx);
        auto[var_expr, code_expr] = sdt_expr(child[5].get());

        Appender{code}
            .append(code_idx)
            .append(code_array)
            .append(code_expr)
            .append({{.op     = Op::kStore,
                      .args_m = {.var = var_expr, .mem = var_array}}});
        return;
    } else if (child[0].get()->id == '$') { // set memory
        // expr_stmt : '$' expr '=' expr ';'
        assert(child[1].get()->id == syntax::expr);
        assert(child[3].get()->id == syntax::expr);

        auto[var_expr1, code_expr1] = sdt_expr(child[1].get());
        auto[var_expr3, code_expr3] = sdt_expr(child[3].get());
        Appender{code}
            .append({code_expr1})
            .append({code_expr3})
            .append(
                {QuadTuple{.op     = Op::kStore,
                           .args_m = {.var = var_expr3, .mem = var_expr1}}});
        return;
    } else if (child[0].get()->id == syntax::IDENT && // function call
               child[2].get()->id == syntax::args_) {
        // expr_stmt : IDENT '(' args_ ')' ';'
        assert(child[2].get()->id == syntax::args_);

        auto func = parse_ident_as_function(child[0].get());

        auto code_args = Code{};
        auto node_args = child[2].get();
        auto var_args  = sdt_args(node_args, code_args);
        auto code_funccall =
            parse_function_call(child[2].get(), func, var_args);

        Appender{code}.append(code_args).append(code_funccall);
        return;

    } else {
        logger.error("expr_stmt pidx={}", node->pidx);
        assert(false);
    }
}

// parse block_stmt
void SdtParser::sdt_block_stmt(APTnode *node, Code &code) {
    // block_stmt : '{' local_decls stmt_list '}' | '{' '}'
    assert(node->id == syntax::block_stmt);
    if (node->child.size() < 4) { return; }

    auto func             = func_domains_.back()->back();
    auto local_var_domain = vector<Var *>{};

    var_domains_.push_back(&local_var_domain);
    initializer_domains_.push_back(&code);

    sdt_decl(
        node->child[1].get()); // intializer code will be appended into code
    sdt_stmt(node->child[2].get(), code);

    initializer_domains_.pop_back();
    var_domains_.pop_back();

    Appender{func->localVars}.append(local_var_domain);
}

void SdtParser::sdt_if_stmt(APTnode *node, Code &code) {
    // if_stmt : IF '(' expr ')' stmt %prec UMINUS
    //         | IF '(' expr ')' stmt ELSE stmt %prec MPR
    assert(node->id == syntax::if_stmt);
    bool has_else = (node->child.size() > 5);

    auto lbl_then =
        assign_new_label({.name = counter_.assign_unique_name("if.then")});
    auto lbl_else =
        assign_new_label({.name = counter_.assign_unique_name("if.else")});
    auto lbl_end =
        assign_new_label({.name = counter_.assign_unique_name("if.end")});

    auto[var_expr, code_expr] = sdt_expr(node->child[2].get());
    auto code_stmt_then       = Code{};
    auto code_stmt_else       = Code{};
    sdt_stmt(node->child[4].get(), code_stmt_then);
    if (has_else) { sdt_stmt(node->child[6].get(), code_stmt_else); }

    Appender{code}
        .append(code_expr)
        .append({{.op     = Op::kBranch,
                  .args_j = {.var   = var_expr,
                             .addr1 = lbl_then,
                             .addr2 = has_else ? lbl_else : lbl_end}}})
        .append({{.op = Op::kLabel, .args_j = {.addr1 = lbl_then}}})
        .append(code_stmt_then)
        .append({{.op = Op::kGoto, .args_j = {.addr1 = lbl_end}}});

    if (has_else) {
        Appender{code}
            .append({{.op = Op::kLabel, .args_j = {.addr1 = lbl_else}}})
            .append(code_stmt_else)
            .append({{.op = Op::kGoto, .args_j = {.addr1 = lbl_end}}});
    }
    Appender{code}.append({{.op = Op::kLabel, .args_j = {.addr1 = lbl_end}}});
}

void SdtParser::sdt_while_stmt(APTnode *node, Code &code) {
    // while_stmt : WHILE_IDENT '(' expr ')' stmt
    assert(node->id == syntax::while_stmt);

    auto lbl_cond =
        assign_new_label({.name = counter_.assign_unique_name("while.cond")});
    auto lbl_body =
        assign_new_label({.name = counter_.assign_unique_name("while.body")});
    auto lbl_end =
        assign_new_label({.name = counter_.assign_unique_name("while.end")});

    auto[var_expr, code_expr] = sdt_expr(node->child[2].get());
    auto code_stmt            = Code{};

    lbl_continue_stack_.emplace(lbl_cond);
    lbl_break_stack_.emplace(lbl_end);
    sdt_stmt(node->child[4].get(), code_stmt);
    lbl_continue_stack_.pop();
    lbl_break_stack_.pop();

    Appender{code}
        .append({{.op = Op::kLabel, .args_j = {.addr1 = lbl_cond}}})
        .append(code_expr)
        .append({{.op     = Op::kBranch,
                  .args_j = {.var   = var_expr,
                             .addr1 = lbl_body,
                             .addr2 = lbl_end}}})
        .append({{.op = Op::kLabel, .args_j = {.addr1 = lbl_body}}})
        .append(code_stmt)
        .append({{.op = Op::kGoto, .args_j = {.addr1 = lbl_cond}}})
        .append({{.op = Op::kLabel, .args_j = {.addr1 = lbl_end}}});
}

void SdtParser::sdt_return_stmt(APTnode *node, Code &code) {
    // return_stmt : RETURN ';' | RETURN expr ';'
    assert(node->id == syntax::return_stmt);
    bool has_retval = (node->child[1].get()->id == syntax::expr);

    auto lbl_return  = lbl_return_stack_.top();
    auto var_returns = var_returns_stack_.top();

    if (has_retval) {
        assert(var_returns.size() >= 1);
        auto[var_expr, code_expr] = sdt_expr(node->child[1].get());
        Appender{code}.append(code_expr);
        code.emplace_back(
            QuadTuple{.op     = Op::kStore,
                      .args_m = {.var = var_expr, .mem = var_returns[0]}});
    }
    code.emplace_back(
        QuadTuple{.op = Op::kGoto, .args_j = {.addr1 = lbl_return}});
}

void SdtParser::sdt_continue_stmt(APTnode *node, Code &code) {
    // continue_stmt : CONTINUE ';'
    assert(node->id == syntax::continue_stmt);

    auto lbl_cond = lbl_continue_stack_.top();
    code.emplace_back(
        QuadTuple{.op = Op::kGoto, .args_j = {.addr1 = lbl_cond}});
}

void SdtParser::sdt_break_stmt(APTnode *node, Code &code) {
    // break_stmt : BREAK ';'
    assert(node->id == syntax::break_stmt);

    auto lbl_end = lbl_break_stack_.top();
    code.emplace_back(QuadTuple{.op = Op::kGoto, .args_j = {.addr1 = lbl_end}});
}

// ---------- public ----------

SdtParser &SdtParser::parse() {
    // initialization
    ir_.clear();
    auto globalInitalizer = Code{}; // should not be used

    var_domains_.clear();
    func_domains_.clear();

    lbl_continue_stack_ = {};
    lbl_break_stack_    = {};
    lbl_return_stack_   = {};
    var_returns_stack_  = {};
    counter_.clear();

    // syntax directed translation
    var_domains_.emplace_back(&ir_.globalVars);
    func_domains_.emplace_back(&ir_.globalFuncs);
    initializer_domains_.emplace_back(&globalInitalizer);

    sdt_decl(root_); // core

    initializer_domains_.pop_back();
    func_domains_.pop_back();
    var_domains_.pop_back();

    assert(globalInitalizer.size() == 0);

    logger.info("syntax-directed-translation complete successfully");
    return *this;
}

Ir SdtParser::get() { return std::move(ir_); }

} // namespace krill::minic
