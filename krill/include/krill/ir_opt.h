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
#include <tuple>
using std::pair, std::vector, std::map, std::set, std::tuple;

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

// DAG Node
struct DagNode {
    Op       op   = Op::kNop;
    DagNode *src1 = nullptr;
    DagNode *src2 = nullptr;

    // the reserved one in all same-name-variabels
    Var *var;
    // record expression-related quad-tuples that defines this DAG node
    vector<QuadTuple *> defined;
    vector<QuadTuple *> referenced;
    // used <=> write into memory
    bool used = false;

    void assignUsed() {
        if (used) { return; }
        if (src1 != nullptr) { src1->assignUsed(); }
        if (src2 != nullptr) { src2->assignUsed(); }
        used = true;

        krill::log::logger.debug("    node {} used",
                         var_name(var));
    }
};

class IrOptimizer {
  private:
    using Expr = tuple<Op, DagNode *, DagNode *>;

    Ir &             ir_;

    // BasicBlockGraph  blocks_;
    // PtrPool<DagNodeas> nodes;

    void livenessAnalysis(const QuadTuple &q, vector<Var *> &defs,
                          vector<Var *> &uses);

    BasicBlockGraph getBasicBlocks(const Code &code);
    BasicBlockGraph simplifyBasicBlocks(const BasicBlockGraph &blockGraph);
    InfGraph        getGlobalInfGraph(const BasicBlockGraph &blockGraph);
    InfGraph        getLocalInfGraph(const BasicBlock &block,
                                     const set<Var *> &globalRegVars);
    map<Var *, int> coloring(const InfGraph &infGraph);
    PtrPool<DagNode> buildDAG(Code &code);
    void updateToIr(Code &code, const BasicBlockGraph &blockGraph);

  public:
    IrOptimizer(Ir &ir) : ir_(ir){};

    IrOptimizer &annotateInfo();
    IrOptimizer &propagateConstValue();
    IrOptimizer &assignRegs();
    IrOptimizer &eliminateCommonSubExpr();
};

} // namespace krill::ir

#endif