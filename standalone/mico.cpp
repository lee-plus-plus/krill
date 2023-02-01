#include "fmt/format.h"
#include "krill/ir.h"
#include "krill/ir_opt.h"
#include "krill/minic.h"
#include "krill/minic_sdt.h"
#include "krill/mips_backend.h"
#include "krill/utils.h"
#include <cstring>
#include <cxxopts.hpp>
#include <fmt/color.h>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <vector>
using namespace std;
using namespace krill::type;
using namespace krill::utils;
using namespace krill::runtime;
using namespace krill::minic;

using krill::log::logger;

void parse_example(istream &input, ostream &output) {
    // mini-C parsing
    auto parser  = krill::minic::MinicParser();
    parser.parseAll(input);
    auto aptRoot = parser.getAptNode();
    // syntax-directed translation
    auto sdtParser = krill::minic::SdtParser(aptRoot.get());
    auto ir        = sdtParser.parse().get();

    // optimization
    auto irOptimizer = krill::ir::IrOptimizer(ir);
    irOptimizer
    .annotateInfo()
    .propagateConstValue()
    .eliminateCommonSubExpr()
    .assignRegs();

    // mips code generation
    auto mipsGenerator = krill::mips::MipsGenerator(ir);
    mipsGenerator.parse();
    // output mips code
    output << mipsGenerator.gen() << "\n";
}

// mips code generation
void parse(istream &input, ostream &output, int opt_level, bool ast_only,
           bool ir_only, bool compile_only) {
    auto parser  = MinicParser();
    auto grammar = minicGrammar;

    // mini-C parsing
    parser.parseAll(input);
    auto root = parser.getAptNode();

    if (ast_only) {
        output << getASTstr(root, grammar) << "\n";
        exit(0);
    }

    // syntax-directed translation
    auto sdtParser = krill::minic::SdtParser(root.get());
    auto ir        = sdtParser.parse().get();
    to_string(ir.code());

    if (opt_level == 0 && ir_only) {
        output << to_string(ir.code()) << "\n";
        exit(0);
    }

    // optimization
    auto irOptimizer = krill::ir::IrOptimizer(ir);
    irOptimizer.annotateInfo();

    if (opt_level >= 1) {
        irOptimizer.propagateConstValue();
    }
    if (opt_level == 1 && ir_only) {
        output << to_string(ir.code()) << "\n";
        exit(0);
    }
    if (opt_level >= 2) {
        irOptimizer.eliminateCommonSubExpr();
    }
    if (opt_level == 2 && ir_only) {
        output << to_string(ir.code()) << "\n";
        exit(0);
    }

    irOptimizer.assignRegs();

    // mips code generation
    auto mipsGenerator = krill::mips::MipsGenerator(ir);
    mipsGenerator.parse();

    if (compile_only) {
        output << mipsGenerator.gen() << "\n";
        exit(0);
    }
}

int main(int argc, char **argv) {
    cxxopts::Options opts("mico", "mico: minisys compiler from mini-c to mips");

    opts.positional_help("file");
    opts.show_positional_help();

    opts.set_width(70).set_tab_expansion().allow_unrecognised_options();
    opts.add_options()("O,Opt", "Optimization level between 0,1,2.",
                       cxxopts::value<int>()->default_value("2"));
    opts.add_options()("A,Ast", "Abstract-parsing-tree only.");
    opts.add_options()("I,Ir",
                       "Intermediate Representation only.");
    opts.add_options()("S", "Compile to Mips.",
                       cxxopts::value<bool>()->default_value("true"));
    opts.add_options()("i,input", "Input mini-C file.",
                       cxxopts::value<string>());
    opts.add_options()("o,output", "Output file.",
                       cxxopts::value<string>()->default_value("stdout"));
    opts.add_options()("v,verbose", "Verbose mode.");
    opts.add_options()("h,help", "Show help.");

    opts.parse_positional({"input", "output", "positional"});

    auto result = opts.parse(argc, argv);

    if (result.count("help")) {
        std::cerr << opts.help();
        exit(0);
    }

    bool is_legal = true;

    int    opt_level       = result["Opt"].as<int>();
    bool   ast_only        = result["Ast"].as<bool>();
    bool   ir_only         = result["Ir"].as<bool>();
    bool   compile_only    = result["S"].as<bool>();
    string input_filename  = result["input"].as<string>();
    string output_filename = result["output"].as<string>();
    bool   verbose         = result["verbose"].as<bool>();

    ifstream input_file;
    ofstream output_file;
    istream *input = nullptr;
    ostream *output = nullptr;

    if (verbose) {
        krill::log::sink_cerr->set_level(spdlog::level::debug);
        logger.info("Verbose Mode");
    } else {
        krill::log::sink_cerr->set_level(spdlog::level::warn);
    }

    if (result.count("input") == 0) {
        std::cerr << "mico: \033[31mfatal error:\033[0m no input files\n";
        is_legal = false;
    } else {
        input_file = ifstream(input_filename);
        if (!input_file) {
            std::cerr << fmt::format("mico: \033[31merror:\033[0m {}: No "
                                     "such file or directory\n",
                                     input_filename);
            is_legal = false;
        }
        input = &input_file;
        spdlog::debug("Input: {}", input_filename);
    }

    if (output_filename == "stdout") {
        output = &std::cout;
        logger.info("Output: {}", "stdout");
    } else {
        if (!output_file) {
            std::cerr << fmt::format(
                "mico: \033[31merror:\033[0m {}: No such file or directory\n",
                output_filename);
            is_legal = false;
        } else {
            output_file = ofstream(output_filename);
            output = &output_file;
            logger.info("Output: {}", output_filename);
        }
    }

    if (!is_legal) { exit(1); }

    parse(*input, *output, opt_level, ast_only, ir_only, compile_only);

    return 0;
}
