#ifndef MINIC_SDT_H
#define MINIC_SDT_H
// #include "krill/defs.h"
#include "krill/ir.h"
#include "krill/minic.h"
#include "krill/syntax.h"
#include "krill/utils.h"
#include <iostream>
#include <memory>
#include <stack>
#include <vector>
using namespace krill::minic;
using namespace krill::ir;

namespace krill::minic {

// for unique naming
struct NameCounter {
    AttrDict cnt;

    string assign_unique_name(const string &s) {
        int &idx = (!cnt.Has<int>(s)) ? cnt.RefN<int>(s) : cnt.Ref<int>(s);
        return s + "." + krill::utils::to_string(idx++);
    }
    void clear() { cnt.Clear(); }
};

// syntax directed translation parser
class SdtParser {
  private:
    // domain std::stack, make it easier in sdt
    // notice: declarations will be directly appended onto the back of domain
    std::vector<std::vector<Var *> *>  var_domains_;
    std::vector<std::vector<Func *> *> func_domains_;

    // for backpatching
    std::stack<Lbl *>              lbl_continue_stack_;
    std::stack<Lbl *>              lbl_break_stack_;
    std::stack<Lbl *>              lbl_return_stack_;
    std::stack<std::vector<Var *>> var_returns_stack_;

    // for unique naming
    NameCounter counter_;

    // result
    Ir ir_;

    Var * assign_new_variable(const Var &base);
    Lbl * assign_new_label(const Lbl &base);
    Func *assign_new_function(const Func &base);

    // find declaration from local to global domains
    // if failed, return nullptr
    Var * find_varible_by_name(const string &varname);
    Func *find_function_by_name(const string &funcname);

    // simple parsing
    int                parse_int_literal(APTnode *node);
    TypeSpec           parse_basetype(APTnode *node);
    Var *              parse_var_decl(APTnode *node);
    std::vector<Var *> parse_params(APTnode *node);
    Var *              parse_ident_as_variable(APTnode *node);
    Func *             parse_ident_as_function(APTnode *node);
    Code parse_function_call(Func *func, const std::vector<Var *> &var_args);

    pair<Var *, Code> parse_array_element(Var *var_ident, Var *var_idx);

    // complex parsing
    void               sdt_decl(APTnode *node);
    void               sdt_stmt(APTnode *node, Code &code);
    void               sdt_global_func_decl(APTnode *node);
    void               sdt_global_var_decl(APTnode *node);
    void               sdt_local_var_decl(APTnode *node);
    std::vector<Var *> sdt_args(APTnode *node, Code &code);

    pair<Var *, Code> sdt_expr(APTnode *node);

    void sdt_expr_stmt(APTnode *node, Code &code);
    void sdt_if_stmt(APTnode *node, Code &code);
    void sdt_while_stmt(APTnode *node, Code &code);
    void sdt_return_stmt(APTnode *node, Code &code);
    void sdt_continue_stmt(APTnode *node, Code &code);
    void sdt_break_stmt(APTnode *node, Code &code);

  public:
    void parse(shared_ptr<APTnode> &node);
    Ir   get();
};

// syntax directed translation
Ir getIrFromAPT(shared_ptr<APTnode> &node);

} // namespace krill::minic
#endif