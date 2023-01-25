#include "fmt/format.h"
#include "krill/ir.h"
#include "krill/ir_opt.h"
#include "krill/minic.h"
#include "krill/minic_sdt.h"
#include "krill/mips_backend.h"
#include "krill/utils.h"
#include <cstring>
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

using krill::log::logger;

// lexical parsing test
void testLexicalParsing() {
    auto lexicalParser = MinicLexicalParser();
    cerr << "input characters (end with ^d): \n"
            "e.g., int main(void) {\\n} \n";

    vector<Token> tokens = lexicalParser.parseAll(cin);
    for (auto token : tokens) {
        cout << fmt::format(
            "[{:d} \"{}\"] ", token.id,
            fmt::format(fmt::emphasis::underline, "{}", unescape(token.lval)));
    }
    cout << "\n";
}

// lexical parsing test (full)
void testSyntaxParsing() {
    auto grammar       = minicGrammar;
    auto syntaxParser = MinicSyntaxParser();

    cerr << "terminal set of grammar:\n  ";
    for (int i : grammar.terminalSet) {
        string name = grammar.symbolNames[i];
        cerr << fmt::format("{} ", name);
    }
    cerr << "\n";

    auto symbolId = reverse(grammar.symbolNames);

    cerr << "input terminals sequence (end with END_ and enter): \n"
            "e.g., INT IDENTIFIER '(' '}' '{' '}' END_\n";
    while (true) {
        syntaxParser.clear();
        cerr << "> ";
        string line;

        getline(cin, line);
        if (cin.eof()) { break; }
        if (line.size() == 0) { continue; }

        vector<string> tokenNames = split(line, " ");
        vector<Token>  tokens;
        bool           isNameOK = true;
        for (auto name : tokenNames) {
            if (symbolId.count(name) == 0) {
                cerr << fmt::format(
                    "unmatched token name \"{}\"\n",
                    fmt::format(fmt::emphasis::underline, "{}", name));
                isNameOK = false;
                break;
            }
            int id = symbolId.at(name);
            tokens.push_back({.id = id, .lval = "[" + name + "]"});
        }
        if (!isNameOK) { continue; }
        tokens.push_back(END_TOKEN);

        syntaxParser.parseAll(tokens);
        cout << syntaxParser.getAPTstr();

        cout << "\n";
    }
}

// syntax parsing test
void testFullLexicalParsing() {
    auto parser = MinicParser();
    auto grammar = minicGrammar;
    cerr << "input characters (end with ^d): \n"
            "e.g., int main(void) {\\n} \n";

    parser.parseAll(cin);

    auto nodes = parser.getNodes();
    for (auto node : nodes) {
        cout << fmt::format(
            "[{} \"{}\"] ", grammar.symbolNames.at(node.id),
            fmt::format(fmt::emphasis::underline, "{}",
                        unescape(node.attr.Get<string>("lval"))));
    }
    cout << "\n";
}

// lexical & syntax parsing test
void testFullLexicalSyntaxParsing() {
    auto parser = MinicParser();
    auto grammar = minicGrammar;
    cerr << "input characters (end with ^d): \n"
            "e.g., int main(void) {\\n} \n";

    parser.parseAll(cin);

    auto nodes = parser.getNodes();
    auto root  = parser.getAptNode();
    for (auto node : nodes) {
        cout << fmt::format("{} ", grammar.symbolNames.at(node.id));
    }
    logger.debug("annotated parsing tree:\n{}", getAPTstr(root, grammar));

    cout << "\n";
    cout << getASTstr(root, grammar);
    cout << "\n";
}

// intermediate code generation
void testIrGeneration() {
    auto parser = MinicParser();
    cerr << "input characters (end with ^d): \n"
            "e.g., int main(void) {\\n} \n";

    // get annotated parsing tree
    parser.parseAll(cin); 
    shared_ptr<APTnode> root = parser.getAptNode(); 

    krill::log::sink_cerr->set_level(spdlog::level::debug);

    // syntax-directed translation
    auto sdtParser = krill::minic::SdtParser(root.get());
    auto ir        = sdtParser.parse().get();
    auto irCode    = ir.code();

    // side-effect: record name for temporary variables
    // good for debug
    to_string(ir.code());

    auto irOptimizer = krill::ir::IrOptimizer(ir);
    irOptimizer.annotateInfo();

    // show intermediate representation code
    cout << to_string(ir.code()) << "\n";
}

// intermediate code generation
void testIrOptimization() {
    auto parser = MinicParser();
    cerr << "input characters (end with ^d): \n"
            "e.g., int main(void) {\\n} \n";

    // get annotated parsing tree
    parser.parseAll(cin); 
    shared_ptr<APTnode> root = parser.getAptNode(); 


    // syntax-directed translation
    auto sdtParser = krill::minic::SdtParser(root.get());
    auto ir        = sdtParser.parse().get();
    auto irCode    = ir.code();

    // side-effect: record name for temporary variables
    // good for debug
    cout << to_string(ir.code()) << "\n";

    auto irOptimizer = krill::ir::IrOptimizer(ir);
    irOptimizer.annotateInfo();

    krill::log::sink_cerr->set_level(spdlog::level::debug);
    irOptimizer.eliminateCommonSubExpr();

    // show intermediate representation code
    cout << to_string(ir.code()) << "\n";
}

// mips code generation
void testMipsGeneration() {
    auto parser = MinicParser();
    cerr << "input characters (end with ^d): \n"
            "e.g., int main(void) {\\n} \n";

    // get annotated parsing tree
    parser.parseAll(cin); // here is a critical bug
    // auto root1 = parser.root_; // magic, don't remove
    auto root = parser.getAptNode();

    // syntax-directed translation
    auto sdtParser = krill::minic::SdtParser(root.get());
    auto ir        = sdtParser.parse().get();
    auto irCode    = ir.code();

    // side-effect: record name for temporary variables
    // good for debug
    to_string(irCode);

    // optimization
    auto irOptimizer = krill::ir::IrOptimizer(ir);
    irOptimizer.annotateInfo().eliminateCommonSubExpr();
    irOptimizer.assignRegs();

    // mips code generation
    auto mipsGenerator = krill::mips::MipsGenerator(ir);
    mipsGenerator.parse();
    auto mipsCode      = mipsGenerator.gen();

    // this code may lead to previous runtime error (weird)
    cout << mipsCode << "\n";
}

const char usage[] = "usage: test_minic {-l|-L|-s}\n"
                     "        -l    lexical parsing test\n"
                     "        -L    lexical parsing test (full)\n"
                     "        -s    syntax parsing test\n"
                     "        -Ls   lexical & syntax parsing test\n"
                     "        -I    intermediate code generation\n"
                     "        -I1   intermediate code generation with optimization\n"
                     "        -S    mips code generation\n";

int main(int argc, char **argv) {
    if (argc != 2) {
        cerr << usage;
        return 1;
    }

    if (strcmp(argv[1], "-l") == 0) {
        testLexicalParsing();
        cerr << "done!\n";
    } else if (strcmp(argv[1], "-L") == 0) {
        testFullLexicalParsing();
        cerr << "done!\n";
    } else if (strcmp(argv[1], "-s") == 0) {
        testSyntaxParsing();
        cerr << "done!\n";
    } else if (strcmp(argv[1], "-Ls") == 0) {
        testFullLexicalSyntaxParsing();
        cerr << "done!\n";
    } else if (strcmp(argv[1], "-I") == 0) {
        testIrGeneration();
        cerr << "done!\n";
    } else if (strcmp(argv[1], "-I1") == 0) {
        testIrOptimization();
        cerr << "done!\n";
    } else if (strcmp(argv[1], "-S") == 0) {
        testMipsGeneration();
        cerr << "done!\n";
    } else {
        cerr << usage;
        return 1;
    }
    return 0;
}
