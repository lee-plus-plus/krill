#ifndef IR_OPT_H
#define IR_OPT_H
#include "krill/ir.h"
#include "krill/utils.h"
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <stack>
#include <vector>
using std::pair, std::vector, std::map, std::set;

namespace krill::ir {

template <typename T> using EdgeSet = set<pair<T, T>>;

struct BasicBlock {
    Lbl *lbl;
    Code code;
};

struct BasicBlockGraph {
    vector<BasicBlock> blocks;
    EdgeSet<int>       edges;
};

// Interference Graph
struct InfGraph {
    set<Var *>     vars;
    EdgeSet<Var *> edges;
};

class IrOptimizer {
  private:
    Ir &            ir_;
    BasicBlockGraph blocks_;

    void livenessAnalysis(const QuadTuple &q, vector<Var *> &defs,
                          vector<Var *> &uses);

    BasicBlockGraph getBasicBlocks(const Code &code);
    InfGraph        getGlobalInfGraph(const BasicBlockGraph &blockGraph);
    InfGraph        getLocalInfGraph(const BasicBlock &block,
                                     const set<Var *> &globalRegVars);
    map<Var *, int> coloring(const InfGraph &infGraph);

  public:
    IrOptimizer(Ir &ir): ir_(ir) {};

    IrOptimizer &annotateInfo();
    IrOptimizer &assignRegs();
};

} // namespace krill::ir

#endif