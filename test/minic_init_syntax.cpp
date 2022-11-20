#include "fmt/format.h"
#include "krill/minic.h"
#include "krill/utils.h"
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

// afunc_ = defaultActionFunc;
// rfunc_0 = defaultReduceFunc;

// syntax ACTION
Afunc afunc_ = defaultActionFunc;
// 0: cpp_file -> translation_unit
Rfunc rfunc_0 = defaultReduceFunc;
// ...
// 236: declaration_list -> declaration
Rfunc rfunc_236 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 237: declaration_list -> declaration_list declaration
Rfunc rfunc_237 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

extern void initSyntaxParser() {
    vector<Rfunc> r_funcs_ = {
        rfunc_0, /* ... */ rfunc_236, rfunc_237};
    // Actions
    minicSyntaxParser.actionFunc_ = afunc_;
    minicSyntaxParser.reduceFunc_ = r_funcs_;
}