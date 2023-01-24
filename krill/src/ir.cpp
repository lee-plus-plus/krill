#include "krill/ir.h"
#include "krill/utils.h"
#include "fmt/format.h"
#include "krill/defs.h"
#include <cassert>
#include <iostream>
#include <map>
#include <set>
#include <vector>
#include <string>
using namespace std;
using namespace krill::type;
using namespace krill::utils;


namespace krill::ir {

int Var::TypeDecl::baseSize() const {
    return ((basetype == TypeSpec::kInt32) ? 4 : 0);
}

int Var::TypeDecl::size() const {
    return apply_reduce(
        shape, baseSize(),
        [](int size, int dimSize) -> int { return size * dimSize; });
};

string Var::TypeDecl::str() const {
    stringstream ss;
    ss << enum_name(basetype);
    for (const auto &dimSize : shape) {
        ss << "[" << dimSize << "]";
    }
    return ss.str();
}

bool Var::TypeDecl::operator==(const TypeDecl &td) const {
    return std::tie(basetype, shape) == std::tie(td.basetype, td.shape);
}

bool Var::TypeDecl::operator!=(const TypeDecl &td) const {
    return std::tie(basetype, shape) != std::tie(td.basetype, td.shape);
}

bool Var::TypeDecl::operator<(const TypeDecl &td) const {
    return std::tie(basetype, shape) < std::tie(td.basetype, td.shape);
}

string Var::fullname() const {
    return type.str() + " " + name;
}

string Func::TypeDecl::str() const {
    assert(returnsType.size() <= 1);

    stringstream ss;
    ss << (returnsType.size() ? returnsType[0].str() : enum_name(TypeSpec::kVoid));
    ss << " " << funcName << "(";
    for (const auto &paramType : paramsType) {
        ss << paramType.str() << ", ";
    }
    ss << ")";
    return ss.str();
}

bool Func::TypeDecl::operator==(const Func::TypeDecl &td) const {
    return std::tie(funcName, returnsType, paramsType) ==
           std::tie(td.funcName, td.returnsType, td.paramsType);
}

bool Func::TypeDecl::operator!=(const Func::TypeDecl &td) const {
    return std::tie(funcName, returnsType, paramsType) !=
           std::tie(td.funcName, td.returnsType, td.paramsType);
}

bool Func::TypeDecl::operator<(const Func::TypeDecl &td) const {
    return std::tie(funcName, returnsType, paramsType) <
           std::tie(td.funcName, td.returnsType, td.paramsType);
}

string Func::fullname() const {
    auto getTypeStr = [](Var *var) -> string { return var->type.str(); };
    auto getFullname = [](Var *var) -> string { return var->fullname(); };

    stringstream ss;
    ss << fmt::format(
        "{} {}({})",
        fmt::join((returns.size() != 0)
                      ? apply_map(returns, getTypeStr)
                      : vector<string>{string{enum_name(TypeSpec::kVoid)}},
                  ", "),
        name,
        fmt::join((params.size() != 0)
                      ? apply_map(params, getFullname)
                      : vector<string>{string{enum_name(TypeSpec::kVoid)}},
                  ", "));
    return ss.str();
}

Func::TypeDecl Func::type() const {
    // tuple<string, vector<TypeDecl>, vector<TypeDecl>>; // <name, ret, params>
    Func::TypeDecl funcType;
    auto extractType = [](Var *var) -> Var::TypeDecl { return var->type; };
    return Func::TypeDecl {
        .funcName    = this->name,
        .returnsType = apply_map(this->returns, extractType),
        .paramsType  = apply_map(this->params, extractType)
    };
}

// ---------- Ir ----------

void Ir::clear() {
    variables.clear();
    labels.clear();
    functions.clear();
    globalVars.clear();
    globalFuncs.clear();
}

Code Ir::code() {
    Code code;
    for (auto &var : globalVars) {
        code.emplace_back(QuadTuple{
            .op = Op::kGlobal, .args_d = {.var = var, .size = var->type.size()}
        });
    }
    for (auto &func : globalFuncs) {
        if (func->code.has_value()) {
            Appender{code}
            .append({{.op = Op::kFuncBegin, .args_f = {.func = func}}})
            .append(func->code.value())
            .append({{.op = Op::kFuncEnd, .args_f = {.func = func}}});
        } else {
            // not linked yet
            // do nothing
        }
    }
    return code;
}

// ---------- to_string ----------

// readable intermediate representation generator
inline map<Var *, string> var_name_;
inline int tempVarIdx = 1;

string var_name(Var *var) {
    if (var_name_.count(var)) {
        return var_name_.at(var);
    } else {
        if (var->name != "") {
            string varname = string{"%"} + var->name;
            var_name_[var] = varname;
            return varname;
        } else {
            string varname = string{"%"} + std::to_string(tempVarIdx++);
            var_name_[var] = varname;
            return varname;
        }
    }
}

string lbl_name(Lbl *lbl) {
    return string{"@"} + lbl->name;
}

string lbl_fullname(Lbl *lbl) {
    return lbl->name;
}

string func_name(Func *func) {
    return string{"@"} + func->name;
}

string func_fullname(Func *func) {
    return func->fullname();
}

string to_string(const QuadTuple &q) {
    stringstream ss;
    switch (q.op) {
        case Op::kNop:
            ss << fmt::format("{:s}", enum_name(q.op));
            break;
        case Op::kAssign:
            ss << fmt::format("{} = {:s} {:d}", var_name(q.args_i.var),
                              enum_name(q.op), int(q.args_i.cval));
            break;
        case Op::kAdd:
        case Op::kSub:
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
            ss << fmt::format("{} = {:s} {} {}", var_name(q.args_e.dest),
                              enum_name(q.op), var_name(q.args_e.src1),
                              var_name(q.args_e.src2));
            break;
        case Op::kAlloca:
        case Op::kGlobal:
            ss << fmt::format("{} = {:s} ({} byte)", var_name(q.args_d.var),
                              enum_name(q.op), q.args_d.size);
            break;
        case Op::kLoad:
            ss << fmt::format("{} = {:s} ({})", var_name(q.args_m.var),
                              enum_name(q.op), var_name(q.args_m.mem));
            break;
        case Op::kStore:
            ss << fmt::format("({}) = {:s} {}", var_name(q.args_m.mem),
                              enum_name(q.op), var_name(q.args_m.var));
            break;
        case Op::kRet:
            ss << fmt::format("{:s}", enum_name(q.op));
            break;
        case Op::kLabel:
            ss << fmt::format("{}:", lbl_fullname(q.args_j.addr1));
            break;
        case Op::kGoto:
            ss << fmt::format("{:s} {}", enum_name(q.op),
                              lbl_name(q.args_j.addr1));
            break;
        case Op::kBranch:
            ss << fmt::format("{:s} ({}) {} {}", enum_name(q.op),
                              var_name(q.args_j.var), lbl_name(q.args_j.addr1),
                              lbl_name(q.args_j.addr2));
            break;
        case Op::kCall:
            ss << fmt::format("{:s} {}", enum_name(q.op), func_name(q.args_f.func));
            break;
        case Op::kParamPut:
            ss << fmt::format("{}<{:d}> {} ", enum_name(q.op), int(q.args_f.idx),
                              var_name(q.args_f.var));
            break;
        case Op::kRetPut:
            ss << fmt::format("{}<{:d}> {} ", enum_name(q.op), int(q.args_f.idx),
                              var_name(q.args_f.var));
            break;
        case Op::kRetGet:
            ss << fmt::format("{} = {:s}<{:d}>", var_name(q.args_f.var),
                              enum_name(q.op), int(q.args_f.idx));
            break;
        case Op::kFuncBegin:
            ss << fmt::format("{} {{", func_fullname(q.args_f.func));
            break;
        case Op::kFuncEnd:
            ss << fmt::format("}}") << "\n";
            break;
        default:
            assert(false);
    }
    return ss.str();
}

string to_string(const Code &code) {
    // initialize
    var_name_.clear();
    var_name_[var_zero] = "%zero";
    tempVarIdx = 1;

    stringstream ss;
    for (const auto &q : code) {
        if (q.op == Op::kFuncBegin) {
            ss << '\n';
        }
        if (q.op != Op::kLabel && q.op != Op::kFuncBegin &&
            q.op != Op::kFuncEnd && q.op != Op::kGlobal) {
            ss << "\t";
        }
        ss << to_string(q) << "\n";
    }
    return ss.str();
}


} // namespace krill::ir