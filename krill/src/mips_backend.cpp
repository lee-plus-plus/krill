#include "fmt/format.h"
#include "krill/mips_backend.h"
#include "krill/utils.h"
#include <cstdlib>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <vector>
using namespace std;
using namespace krill::type;
using namespace krill::utils;
using namespace krill::mips;
using namespace krill::ir;

using krill::log::logger;

// ---------- utils function ----------

string to_hex(int16_t src) {
    stringstream ss;
    ss << std::hex << std::showbase << src;
    return ss.str();
}

int32_t zero_extend(int16_t src) { return static_cast<uint16_t>(src); }

int32_t sign_extend(int16_t src) { return static_cast<int16_t>(src); }

// ---------- one-line-mips code generate ----------

void MipsGenerator::genSection(string src) {
    mips_code_.push_back(to_string(fmt::format("{}", src)));
}

void MipsGenerator::genComment(string src) {
    mips_code_.push_back(to_string(fmt::format("{} ", src)));
}

void MipsGenerator::genData(string src, int size) {
    mips_code_.push_back(to_string(fmt::format("\t{}: .space {}", src, size)));
}

void MipsGenerator::genCode(string op) {
    mips_code_.push_back(to_string(fmt::format("\t{} ", op)));
}

void MipsGenerator::genCode(string op, string src1) {
    mips_code_.push_back(to_string(fmt::format("\t{} \t{} ", op, src1)));
}

void MipsGenerator::genCode(string op, string src1, string src2) {
    mips_code_.push_back(
        to_string(fmt::format("\t{} \t{}, {} ", op, src1, src2)));
}

void MipsGenerator::genCode(string op, string src1, string src2, string src3) {
    if (op == "lw" || op == "sw") {
        mips_code_.push_back(
            to_string(fmt::format("\t{} \t{}, {}({}) ", op, src1, src3, src2)));
    } else {
        mips_code_.push_back(
            to_string(fmt::format("\t{} \t{}, {}, {} ", op, src1, src2, src3)));
    }
}

void MipsGenerator::genCode(string op, string src1, string src2, int src3) {
    if (op == "lw" || op == "sw") {
        mips_code_.push_back(
            to_string(fmt::format("\t{} \t{}, {}({}) ", op, src1, src3, src2)));
    } else {
        mips_code_.push_back(
            to_string(fmt::format("\t{} \t{}, {}, {} ", op, src1, src2, src3)));
    }
}

// void MipsGenerator::genOffsetCode(string op, string src1, string addr,
//                                   string offset) {
//     mips_code_.push_back(
//         to_string(fmt::format("\t{} \t{}, {}({}) ", op, src1, offset, addr)));
// }

// void MipsGenerator::genOffsetCode(string op, string src1, string addr,
//                                   int offset) {
//     mips_code_.push_back(
//         to_string(fmt::format("\t{} \t{}, {}({}) ", op, src1, offset, addr)));
// }

void MipsGenerator::genLabel(string lblname) {
   mips_code_.push_back(
        to_string(fmt::format("{}:", lblname))); 
}

// ---------- translate quad-tuple into mips ----------

void MipsGenerator::genFuncBegin(const QuadTuple &q) {
    auto func     = q.args_f.func;
    auto funcname = func->name;
    auto spOffset = func->info.spOffset;

    genComment(to_string(fmt::format("# {}", func_fullname(func))));
    genLabel(funcname);

    // assign stack space for local variables (example given below: 

    // 8($fp) = param<1> (x)
    // 4($fp) = param<2> (y)
    // 0($fp) = $ra
    // -4($fp) = $fp_last
    // $fp - 8 = $sp
    assert(spOffset.has_value());
    genCode("sw", "$ra", "$sp", 0);
    genCode("sw", "$fp", "$sp", -4);
    genCode("ori", "$fp", "$sp", 0);
    genCode("addiu", "$sp", "$fp", spOffset.value());

    // gen comments for offset meanings
    vector<Var *> localVars;
    Appender{localVars}
        .append(func->params)
        .append(func->returns)
        .append(func->localVars);
    for (const auto &var : localVars) {
        auto varname  = var->name;
        auto fpOffset = var->info.fpOffset;

        assert(var->info.fpOffset.has_value());
        genComment(
            to_string(fmt::format("\t# {}: {}($fp)", varname, fpOffset.value())));
    }
}

void MipsGenerator::genFuncRet(const QuadTuple &q) {
    // pop $sp, $ra, $fp
    genCode("ori", "$sp", "$fp", 0);
    genCode("lw", "$ra", "$fp", 0); // if no func call inside, can be ignored
    genCode("lw", "$fp", "$fp", -4);
    genCode("jr", "$ra");
    genCode("nop");
}

void MipsGenerator::genFuncEnd(const QuadTuple &q) {
    // we assume that each function must has a RETURN
    // and all the work is done when RETURNing
    // so nothing to do at the end of function
    genCode("nop");
}

void MipsGenerator::genFuncCall(const QuadTuple &q) {
    auto func     = q.args_f.func;
    auto funcname = func->name;

    // mips conventions: assume parameters already pushed before
    genCode("addiu", "$sp", "$sp", -4 * func->params.size());

    genComment(to_string(fmt::format("\t# call function {} begin", funcname)));

    genCode("jal", funcname);
    genCode("nop");

    // genCode("lw", "$ra", "$sp", 0);
    genCode("addiu", "$sp", "$sp", +4 * func->params.size());
    genComment(to_string(fmt::format("\t# call function {} end", funcname)));
}

void MipsGenerator::genExprCode(const QuadTuple &q) {
    // r-type
    // TODO: support i-type
    auto v_src1 = q.args_e.src1;
    auto v_src2 = q.args_e.src2;
    auto v_dest = q.args_e.dest;

    assert(v_dest->info.reg.has_value());
    string reg_dest = v_dest->info.reg.value();

    // some operand has a offset value, like $fp + 16, relatived to $fp
    // some others has a label indicating a memory position in data segment
    // expression calculation on such kind of operands is DIFFERENT
    bool is_src1_offset =
        (v_src1->info.fpOffset.has_value() || v_src1->info.memName.has_value());
    bool is_src2_offset =
        (v_src2->info.fpOffset.has_value() || v_src2->info.memName.has_value());
    bool is_src1_reg = v_src1->info.reg.has_value();
    bool is_src2_reg = v_src2->info.reg.has_value();

    if (is_src1_offset && is_src2_offset) {
        // applying such kind of operation on two address is WEIRD
        assert(false);
    }
    if (is_src1_offset && is_src2_reg) {
        // switch position
        // goto (is_src1_offset && is_src2_reg)
        swap(v_src1, v_src2);
        swap(is_src1_offset, is_src2_offset);
        swap(is_src1_reg, is_src2_reg);
    }

    if (is_src1_reg && is_src2_offset) {
        // reg1 + offset2
        // applying other kind of operation on an address is WEIRD
        assert(q.op == Op::kAdd);
        string reg_src1 = v_src1->info.reg.value();

        if (v_src2->info.fpOffset.has_value()) {
            // is fp-offset
            int32_t fpOffset = v_src2->info.fpOffset.value();
            // bug: reg_dest can be the same as reg_src1
            // x genCode("addiu", reg_dest, "$fp", fpOffset);
            genCode("addiu", "$t0", "$fp", fpOffset);
            genCode("addu", reg_dest, "$t0", reg_src1);
        } else if (v_src2->info.memName.has_value()) {
            // is memory-name
            // string memName = v_src2->info.memName.value();
            assert(v_src2->info.memOffset.has_value());
            auto memOffset = v_src2->info.memOffset.value();
            genCode("addiu", reg_dest, reg_src1, memOffset);
        } else {
            assert(false);
        }
    } else if (is_src1_reg && is_src2_reg) {
        // reg1 + reg2
        assert(v_src1->info.reg.has_value());
        assert(v_src2->info.reg.has_value());
        string reg_src1 = v_src1->info.reg.value();
        string reg_src2 = v_src2->info.reg.value();

        switch (q.op) {
        case Op::kAdd:
            genCode("addu", reg_dest, reg_src1, reg_src2);
            break;
        case Op::kSub:
            genCode("subu", reg_dest, reg_src1, reg_src2);
            break;
        case Op::kMult:
            genCode("mult", reg_src1, reg_src2);
            genCode("mflo", reg_dest);
            break;
        case Op::kDiv:
            genCode("div", reg_src1, reg_src2);
            genCode("mflo", reg_dest);
            break;
        case Op::kMod:
            genCode("div", reg_src1, reg_src2);
            genCode("mfhi", reg_dest);
            break;
        case Op::kAnd:
            genCode("and", reg_dest, reg_src1, reg_src2);
            break;
        case Op::kOr:
            genCode("or", reg_dest, reg_src1, reg_src2);
            break;
        case Op::kXor:
            genCode("xor", reg_dest, reg_src1, reg_src2);
            break;
        case Op::kNor:
            genCode("nor", reg_dest, reg_src1, reg_src2);
            break;
        case Op::kLShift:
            genCode("sllv", reg_dest, reg_src1, reg_src2);
            break;
        case Op::kRShift:
            genCode("srlv", reg_dest, reg_src1, reg_src2);
            break;
        case Op::kEq:
            genCode("xor", reg_dest, reg_src1, reg_src2);
            genCode("slti", reg_dest, reg_dest, "1");
            break;
        case Op::kNeq:
            genCode("xor", reg_dest, reg_src1, reg_src2);
            genCode("sltiu", reg_dest, reg_dest, "1");
            genCode("sltiu", reg_dest, reg_dest, "1");
            break;
        case Op::kLeq:
            genCode("subu", reg_dest, reg_src1, reg_src2);
            genCode("slti", reg_dest, reg_dest, "1");
            break;
        case Op::kLt:
            genCode("slt", reg_dest, reg_src1, reg_src2);
            break;
        default:
            assert(false);
        }
    } else {
        assert(false);
    }
}

void MipsGenerator::genLoadStoreCode(const QuadTuple &q) {
    /* (var_m, addr_m) */
    auto v_var = q.args_m.var;
    auto v_mem = q.args_m.mem;
    assert(v_var->info.reg.has_value());
    auto reg_var = v_var->info.reg.value();

    string op;
    if (q.op == Op::kLoad) {
        op = "lw";
    } else if (q.op == Op::kStore) {
        op = "sw";
    } else {
        assert(false);
    }

    if (v_mem->info.fpOffset.has_value()) {
        auto fpOffset = v_mem->info.fpOffset.value();
        genCode(op, reg_var, "$fp", fpOffset);
    } else if (v_mem->info.memName.has_value()) {
        auto memName = v_mem->info.memName.value();
        genCode(op, reg_var, "$zero", memName);
    } else if (v_mem->info.reg.has_value()) {
        auto reg_mem = v_mem->info.reg.value();
        genCode(op, reg_var, reg_mem, 0);
    } else {
        assert(false);
    }
}

void MipsGenerator::genRetPut(const QuadTuple &q) {
    /* (var, idx) */
    auto var = q.args_f.var;
    auto idx = q.args_f.idx;
    assert(var->info.reg.has_value());
    assert(idx == 0);
    string reg_var = var->info.reg.value();

    genCode("ori", "$v0", reg_var, 0);
}

void MipsGenerator::genRetGet(const QuadTuple &q) {
    /* (var, idx) */
    auto var = q.args_f.var;
    auto idx = q.args_f.idx;
    assert(var->info.reg.has_value());
    assert(idx == 0);
    string reg_var = var->info.reg.value();

    genCode("ori", reg_var, "$v0", 0);
}

void MipsGenerator::genParamPut(const QuadTuple &q) {
    /* (var, idx) */
    // # before func call
    // sw $5, 0($sp)        # %5 -> ParamPut<0>
    // sw $6, -4($sp)       # %6 -> ParamPut<1>
    // sw $5, -8($sp)       # %5 -> ParamPut<2>
    // sw $8, -8($fp)       # write value(z) info mem(z)
    // add $sp, $sp, -12    # sp += 12

    // sw $ra, 0($sp)       # call func2 (begin)
    // sw $fp, -4($sp)      # |
    // move $fp, $sp        # |
    // add $sp, $sp, -8     # |
    // jal func2            # |
    // lw $ra, 0($sp)       # | recover $ra
    // add $sp, $sp, 12     # | sp -= 12
    // nop                  # call func2 (end)
    //                      # $fp, $sp should be recoverd

    auto var = q.args_f.var;
    auto idx = q.args_f.idx;
    assert(var->info.reg.has_value());
    string reg_var = var->info.reg.value();

    // TODO: pass param<0-3> by register
    // if (idx <= 3) {
    //     string reg_dest = "$v" + to_string(idx);
    //     genCode("addu", reg_dest, "$zero", reg_var);
    // } else {
    //     genCode("addu", reg_dest, "$zero", reg_var);
    // }
    genCode("sw", reg_var, "$sp", -4 * idx);
}

void MipsGenerator::genBranch(const QuadTuple &q) {
    /* (var_j, addr1, addr2) */
    Var *var   = q.args_j.var;
    Lbl *addr1 = q.args_j.addr1;
    Lbl *addr2 = q.args_j.addr2;
    assert(var->info.reg.has_value());
    string reg_var   = var->info.reg.value();
    string addrname1 = addr1->name;
    string addrname2 = addr2->name;

    genCode("bne", reg_var, "$zero", addrname1);
    genCode("nop");
    genCode("j", addrname2);
    genCode("nop");
}

// assign const value by immediate
void MipsGenerator::genAssign(const QuadTuple &q) {
    auto var = q.args_i.var;
    assert(var->info.reg.has_value());
    auto    reg  = var->info.reg.value();
    int32_t cval = q.args_i.cval;
    int16_t low  = cval & 0xFFFF;
    int16_t high = (cval >> 16) & 0xFFFF;

    if (zero_extend(low) == cval) {
        if (low < -100 || low > 100) {
            genCode("ori", reg, "$zero", to_hex(low));
        } else {
            genCode("ori", reg, "$zero", low);
        }
    } else if (sign_extend(low) == cval) {
        if (low < -100 || low > 100) {
            genCode("addiu", reg, "$zero", to_hex(low));
        } else {
            genCode("addiu", reg, "$zero", low);
        }
    } else {
        genCode("lui", reg, to_hex(high));
        genCode("ori", reg, reg, to_hex(low));
    }
}

// data segment definition
void MipsGenerator::genGlobal(const QuadTuple &q) {
    /* (var, size) */
    auto var     = q.args_d.var;
    auto varname = var->name;
    assert(var->type.size() == q.args_d.size);

    genData(varname, var->type.size());
}

// entry of the whole program, make preparation, jump to the main
void MipsGenerator::genDirector() {
    // auto func_main = get_function_by_name("main").funcLbl.lbl;
    // genFuncCall(QuadTuple{Op::kCall, {.func = lbl_main, .argc = 0}});
    genCode("ori", "$fp", "$zero", "0x8000");
    genCode("ori", "$sp", "$zero", "0x8000");
    // call main
    genCode("jal", "main");
    genCode("nop");

    genLabel("end.end");
    genCode("j", "end.end");
    genCode("nop");
    genCode("nop");
}

// ---------- entry ----------

// dispatcher
void MipsGenerator::genCodes(const QuadTuple &q) {
    // genComment(to_string(fmt::format("\t# {}", to_string(q))));
    // clang-format off
    switch (q.op) {
        case Op::kNop:
            genCode("nop");
            break;
        case Op::kAssign:
            genAssign(q);
            break;
        case Op::kAdd:      case Op::kSub:    case Op::kMult: 
        case Op::kDiv:      case Op::kMod:  
        case Op::kAnd:      case Op::kOr:       case Op::kXor:  case Op::kNor:
        case Op::kLShift:   case Op::kRShift:
        case Op::kEq:       case Op::kNeq:      case Op::kLeq:  case Op::kLt:
            genExprCode(q);
            break;
        case Op::kAlloca:
            // space has been assigned at the start of domain
            // so no action is needed when this quadtuple is read
            break;
        case Op::kGlobal:
            genGlobal(q);
            break;
        case Op::kLoad:
        case Op::kStore:
            genLoadStoreCode(q);
            break;
        case Op::kRet:
            genFuncRet(q);
            break;
        case Op::kLabel:
            genLabel(q.args_j.addr1->name);
            break;
        case Op::kGoto:
            genCode("j", q.args_j.addr1->name);
            genCode("nop");
            break;
        case Op::kBranch:
            genBranch(q);
            break;
        case Op::kCall:
            genFuncCall(q);
            break;
        case Op::kParamPut:
            genParamPut(q);
            break;
        case Op::kRetPut: // put
            genRetPut(q);
            break;
        case Op::kRetGet: // set
            genRetGet(q);
            break;
        case Op::kFuncBegin:
            genFuncBegin(q);
            break;
        case Op::kFuncEnd:
            genFuncEnd(q);
            break;
        default:
            assert(false);
    }
    // clang-format on
}

MipsGenerator &MipsGenerator::parse() {
    Code dataCode;
    Code textCode;
    for (auto &var : ir_.globalVars) {
        dataCode.emplace_back(
            QuadTuple{.op     = Op::kGlobal,
                      .args_d = {.var = var, .size = var->type.size()}});
    }
    for (auto &func : ir_.globalFuncs) {
        if (func->code.has_value()) {
            Appender{textCode}
                .append({{.op = Op::kFuncBegin, .args_f = {.func = func}}})
                .append(func->code.value())
                .append({{.op = Op::kFuncEnd}});
        } else {
            // not linked yet
            // do nothing
        }
    }

    genSection(".DATA");
    for (const auto &ir : dataCode) { genCodes(ir); }
    genSection(".TEXT");
    genDirector(); // direct to main
    for (const auto &ir : textCode) { genCodes(ir); }

    logger.info("mips code generation complete successfully");
    return *this;
}

string MipsGenerator::gen() {
    return
        to_string(fmt::format("{}", fmt::join(mips_code_, "\n")));
}