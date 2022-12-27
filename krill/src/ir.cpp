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

constexpr int TypeSpec::size() const {
    return apply_reduce(
        shape, ((basetype == TypeSpec::kInt32) ? 4 : 0),
        [](int size, int dimSize) -> int { return size * dimSize; });
};

Func::FuncType Func::getTypeDecl() const {
    // tuple<string, vector<TypeDecl>, vector<TypeDecl>>; // <name, ret, params>
    Func::FuncType funcType;
    auto extractTypeDecl = [](Var *var) -> TypeDecl {
        return var->type;
    };
    std::get<0>(funcType) = this->name;
    std::get<1>(funcType) = apply_map(this->returns, extractTypeDecl);
    std::get<2>(funcType) = apply_map(this->params, extractTypeDecl);
    return funcType;
}

// ---




} // namespace krill::ir