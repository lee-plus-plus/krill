#ifndef MIPS_BACKEND_H
#define MIPS_BACKEND_H
#include "krill/ir.h"
#include <vector>
using namespace krill::ir;
using std::vector;

namespace krill::mips {

class MipsGenerator {
  private:
    Ir &           ir_;
    vector<string> mips_code_;

    void genSection(string src);
    void genComment(string src);
    void genData(string src, int size);
    void genCode(string op);
    void genCode(string op, string src1);
    void genCode(string op, string src1, string src2);
    void genCode(string op, string src1, string src2, string src3);
    void genCode(string op, string src1, string src2, int src3);
    void genLabel(string label);

    void genFuncBegin(const QuadTuple &q);
    void genFuncRet(const QuadTuple &q);
    void genFuncEnd(const QuadTuple &q);
    void genFuncCall(const QuadTuple &q);
    void genExprCode(const QuadTuple &q);
    void genLoadStoreCode(const QuadTuple &q);
    void genRetPut(const QuadTuple &q);
    void genRetGet(const QuadTuple &q);
    void genParamPut(const QuadTuple &q);
    void genBranch(const QuadTuple &q);
    void genAssign(const QuadTuple &q);
    void genGlobal(const QuadTuple &q);

    void genDirector();
    void genCodes(const QuadTuple &q);

  public:
    MipsGenerator(Ir &ir) : ir_(ir){};

    MipsGenerator &parse();
    string gen();
};

} // namespace krill::mips
#endif