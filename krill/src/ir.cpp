#include "krill/ir.h"
#include "fmt/format.h"
#include "krill/defs.h"
#include <cassert>
#include <iostream>
#include <map>
#include <set>
#include <vector>
using namespace std;
using namespace krill::type;


namespace krill::ir {

int Var::TypeDecl::size() const {
    return apply_reduce(
        shape, ((basetype == TypeSpec::kInt32) ? 4 : 0),
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

bool Var::TypeDecl::operator==(const TypeDecl &ts) const {
    return std::tie(basetype, shape) == std::tie(ts.basetype, ts.shape);
}

bool Var::TypeDecl::operator!=(const TypeDecl &ts) const {
    return std::tie(basetype, shape) != std::tie(ts.basetype, ts.shape);
}

bool Var::TypeDecl::operator<(const TypeDecl &ts) const {
    return std::tie(basetype, shape) < std::tie(ts.basetype, ts.shape);
}

string Func::TypeDecl::str() const {
    assert(returnsType.size() <= 1);
    ss << (returnsType.size() ? returnsType[0].str() : enum_name(TypeSpec::kVoid));
    ss << " " << funcName << "(";
    for (const auto &paramType : paramsType) {
        ss << paramType.str() << ", ";
    }
    ss << ")";
    return ss.str();
}

bool Func::TypeDecl::operator==(const TypeDecl &ts) const {
    return std::tie(basetype, shape) == std::tie(ts.basetype, ts.shape);
}

bool Func::TypeDecl::operator!=(const TypeDecl &ts) const {
    return std::tie(basetype, shape) != std::tie(ts.basetype, ts.shape);
}

bool Func::TypeDecl::operator<(const TypeDecl &ts) const {
    return std::tie(basetype, shape) < std::tie(ts.basetype, ts.shape);
}

Func::TypeDecl Func::type() const {
    // tuple<string, vector<TypeDecl>, vector<TypeDecl>>; // <name, ret, params>
    Func::TypeDecl funcType;
    auto extractType = [](Var *var) -> Var::TypeDecl { return var->type; };
    return Func::TypeDecl {
        .funcName    = this->name,
        .returnsType = apply_map(this->returns, extractType),
        .paramsType  = apply_map(this->params, extractType)
    }
}

// ---

Var *assign_new_variable(Var &&base) {
    auto ownership = make_unique<Var>(move(base));
    auto var       = ownership.get();
    variables_pool.emplace_back(move(ownership));
    return var;
}

Lbl *assign_new_label(Lbl &&base) {
    auto ownership = make_unique<Lbl>(move(base));
    auto lbl       = ownership.get();
    labels_pool.emplace_back(move(ownership));
    return lbl;
}

Func *assign_new_function(Func &&base) {
    auto ownership = make_unique<Func>(move(base));
    auto func      = ownership.get();
    functions_pool.emplace_back(move(ownership));
    return func;
}


} // namespace krill::ir