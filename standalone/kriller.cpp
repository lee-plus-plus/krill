#include "fmt/format.h"
#include "krill/codegen.h"
#include "krill/grammar.h"
#include "krill/lexical.h"
#include "krill/regex.h"
#include "krill/syntax.h"
#include "krill/utils.h"
#include <cxxopts.hpp>
#include <fmt/color.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

using krill::log::logger;

using namespace std;
using namespace krill::type;
using namespace krill::grammar;
using namespace krill::regex;
using namespace krill::utils;
using namespace krill::codegen;
using namespace krill::runtime;

Grammar parsingYacc(istream &input) {
    vector<string> lines[3];
    // Yacc -> TokenStmts DELIM ProdStmts DELIM SubStmts
    int i = 0;
    // spdlog::info("input token statements");
    while (input.good() && !input.eof()) {
        string line;
        getline(input, line);
        trim(line);

        if (line == "%%") {
            i++;
            if (i == 1) {
                // spdlog::info("input production statements");
            } else if (i == 2) {
                // spdlog::info("input additional statements");
            } else {
                spdlog::error("invalid input");
                assert(false);
            }
            continue;
        } else {
            lines[i].push_back(line);
        }
    }
    // spdlog::info("end input");
    assert(i < 3);
    vector<string> &tokenStmtsLines = lines[0];
    vector<string> &prodStmtsLines  = lines[1];
    // vector<string> &subStmts = lines[2];

    // token statement processing
    int                    priority = -1;
    int                    tokenId  = 258;
    map<string, int>       tokenIds;
    map<string, int>       tokenPriority;
    map<string, Associate> tokenAssociate;
    string                 startToken;

    static const map<string, Associate> toAsso = {
        {"%nonassoc", Associate::kNone},
        {"%left", Associate::kLeft},
        {"%right", Associate::kRight},
    };
    spdlog::info("parsing token statements");
    for (string line : tokenStmtsLines) {
        vector<string> words = split(line, " ");
        if (words.size() == 0) { continue; }
        if (words[0] == "%token") {
            for (int j = 1; j < words.size(); j++) {
                const string &w = words[j];
                assert(tokenIds.count(w) == 0);
                tokenIds[w] = tokenId++;
            }
        } else if (toAsso.count(words[0]) > 0) {
            Associate asso = toAsso.at(words[0]);
            for (int j = 1; j < words.size(); j++) {
                const string &w = words[j];
                assert(tokenPriority.count(w) == 0);
                assert(tokenAssociate.count(w) == 0);
                tokenPriority[w]  = priority;
                tokenAssociate[w] = asso;
            }
            priority -= 1;
        } else if (words[0] == "%start") {
            assert(words.size() == 2);
            startToken = words[1];
        } else {
            spdlog::error("invalid token statement type: {}", words[0]);
            throw runtime_error("parsing failed");
        }
    }

    // productions statement processing
    vector<vector<string>> prodSymbolStrs;

    vector<string> currProdStr;
    stringstream   ss;
    for (string line : prodStmtsLines) { ss << line << " "; }
    spdlog::debug("parsing productions statements");
    while (ss.good() && !ss.eof()) {
        string w;
        ss >> w;
        if (w == "|") {
            prodSymbolStrs.push_back(currProdStr);
            spdlog::debug("  add prod:  {}", fmt::join(currProdStr, " "));
            currProdStr.resize(1); // reserve first elem
        } else if (w == ";") {
            prodSymbolStrs.push_back(currProdStr);
            spdlog::debug("  add prod:  {}", fmt::join(currProdStr, " "));
            currProdStr.resize(0);
        } else if (w == ":") {
            continue; // simply drop it, weak format check
        } else if (slice(w, 0, 2) == "/*") {
            while (slice(w, w.size() - 2) != "*/") {
                ss >> w; // ignore comment, weak format check
                if (!(ss.good() && !ss.eof())) { break; }
            }
        } else {
            currProdStr.push_back(w);
        }
    }

    // SubStmts processing
    // pass

    // summurize
    spdlog::debug("parsing complete");
    spdlog::debug("symbolId: {}", ToString{}(tokenIds));
    spdlog::info("yacc-format grammar parsing complete");
    Grammar grammar(prodSymbolStrs, tokenIds, tokenPriority, tokenAssociate);
    return grammar;
}

void parse_syntax(istream &input, ostream &output, bool is_syntax_yacc,
                  bool is_syntax, bool test_mode, bool gen_mode) {
    assert(input);
    assert(output);

    Grammar grammar;
    if (is_syntax_yacc) {
        grammar = parsingYacc(input);
    } else if (is_syntax) {
        vector<string> strs;
        while (!input.eof()) {
            string line;
            getline(input, line);
            trim(line);
            if (input.eof()) { break; }
            strs.push_back(line);
        }
        grammar = Grammar(strs);
    }

    if (test_mode) {
        auto         actionTable = getLALR1table(grammar);
        SyntaxParser syntaxParser(grammar, actionTable);

        spdlog::info("terminal set of grammar:\n  {}",
                     fmt::join(apply_map(to_vector(grammar.terminalSet),
                                         grammar.symbolNames),
                               " "));

        auto symbolId = reverse(grammar.symbolNames);

        spdlog::info("input to be parsed:");
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
                    spdlog::warn("unmatched token name \033[33m{}\033[0m",
                                 name);
                    isNameOK = false;
                    break;
                }
                int id = symbolId.at(name);
                tokens.push_back({.id = id, .lval = name});
            }
            if (!isNameOK) { continue; }
            tokens.push_back(END_TOKEN);

            try {
                syntaxParser.parseAll(tokens);
                spdlog::info(syntaxParser.getASTstr());
            } catch (exception &e) { spdlog::error(e.what()); }
        }
    } else if (gen_mode) {
        genSyntaxParser(grammar, output);
    }
}

void parse_lexical(istream &input, ostream &output, bool is_lexical,
                   bool test_mode, bool gen_mode) {
    assert(input);
    assert(output);

    spdlog::info("input multiple regexs, end with ^d");
    vector<string> regexs;
    while (!input.eof()) {
        string line;
        getline(input, line);
        trim(line);
        if (input.eof()) { break; }
        regexs.push_back(line);
    }
    spdlog::info("start generate lexical parser ...");

    if (test_mode) {
        stringstream ss;
        for (int i = 0; i < regexs.size(); i++) {
            ss << fmt::format("{:d}) {}\n", i, regexs[i]);
        }
        spdlog::info("given regexs:\n{}", ss.str());

        LexicalParser lexicalParser(regexs);
        spdlog::info("input strings to be parsed:");
        while (true) {
            lexicalParser.clear();
            cerr << "> ";
            string line;

            getline(cin, line);
            if (cin.eof()) { break; }
            if (line.size() == 0) { continue; }

            stringstream ss;
            ss << line;
            try {
                vector<Token> tokens = lexicalParser.parseAll(ss);
                for (auto token : tokens) {
                    spdlog::info("<token {:d}> \"{}\"", token.id, token.lval);
                }
            } catch (exception &e) { spdlog::error(e.what()); }
        }

    } else if (gen_mode) {
        genLexicalParser(regexs, output);
    }
}

int main(int argc, char **argv) {
    cxxopts::Options opts("kriller", "kriller: LALR(1) parser generator");

    opts.positional_help("(PATTERN) (MODE) file");
    opts.show_positional_help();

    opts.set_width(70).set_tab_expansion().allow_unrecognised_options();
    opts.add_options("PATTERN")("S,syntax-yacc",
                                "Syntax parser pattern with yacc format.");
    opts.add_options("PATTERN")("s,syntax", "Syntax parser pattern.");
    opts.add_options("PATTERN")("l,lexical", "Lexical parser pattern.");
    opts.add_options("MODE")(
        "t,test", "Test mode, interact immediately to test the parser.");
    opts.add_options("MODE")("g,gen",
                             "Generator mode, generate code of the parser.");
    opts.add_options()("i,input", "Input file.",
                       cxxopts::value<string>()->default_value("stdin"));
    opts.add_options()("o,output", "Output file.",
                       cxxopts::value<string>()->default_value("stdout"));
    opts.add_options()("v,verbose", "Verbose mode.");
    opts.add_options()("h,help", "Show help.");

    opts.parse_positional({"input", "output", "positional"});

    auto result = opts.parse(argc, argv);

    if (result.count("help")) {
        std::cerr << opts.help({"", "PATTERN", "MODE"});
        exit(0);
    }

    bool is_legal = true;

    int    is_syntax_yacc  = result["syntax-yacc"].as<bool>();
    bool   is_syntax       = result["syntax"].as<bool>();
    bool   is_lexical      = result["lexical"].as<bool>();
    bool   test_mode       = result["test"].as<bool>();
    bool   gen_mode        = result["gen"].as<bool>();
    string input_filename  = result["input"].as<string>();
    string output_filename = result["output"].as<string>();
    bool   verbose         = result["verbose"].as<bool>();

    ifstream input_file;
    ofstream output_file;
    istream *input  = nullptr;
    ostream *output = nullptr;

    spdlog::set_pattern("[%^%l%$] %v");
    spdlog::set_level(spdlog::level::info);
    if (verbose) {
        krill::log::sink_cerr->set_level(spdlog::level::debug);
        spdlog::set_level(spdlog::level::debug);
        spdlog::info("Verbose Mode");
    } else {
        krill::log::sink_cerr->set_level(spdlog::level::warn);
    }

    if (input_filename == "stdin") {
        input = &std::cin;
        spdlog::debug("Input: {}", input_filename);
    } else {
        input_file = ifstream(input_filename);
        if (!input_file) {
            std::cerr << fmt::format("kriller: \033[31merror:\033[0m {}: No "
                                     "such file or directory\n",
                                     input_filename);
            is_legal = false;
        }
        input = &input_file;
        spdlog::debug("Input: {}", input_filename);
    }

    if (output_filename == "stdout") {
        output = &std::cout;
        spdlog::debug("Output: {}", output_filename);
    } else {
        output_file = ofstream(output_filename);
        if (!output_file) {
            std::cerr << fmt::format("kriller: \033[31merror:\033[0m {}: No "
                                     "such file or directory\n",
                                     output_filename);
            is_legal = false;
        }
        output = &output_file;
        spdlog::debug("Output: {}", output_filename);
    }

    if (is_syntax_yacc + is_syntax + is_lexical != 1) {
        std::cerr << fmt::format(
            "kriller: \033[31merror:\033[0m: must specify one "
            "Pattern in syntax-yacc, syntax and "
            "lexical\n");
        is_legal = false;
    }
    if (test_mode + gen_mode != 1) {
        std::cerr << fmt::format(
            "kriller: \033[31merror:\033[0m: must specify one "
            "MODE in test and generator\n");
        is_legal = false;
    }
    if (!is_legal) { exit(1); }

    if (is_syntax_yacc) {
        spdlog::debug("Pattern: syntax-yacc");
    } else if (is_syntax) {
        spdlog::debug("Pattern: syntax");
    } else if (is_lexical) {
        spdlog::debug("Pattern: lexical");
    }

    if (test_mode) {
        spdlog::debug("Mode: test");
    } else if (gen_mode) {
        spdlog::debug("Mode: generator");
    }

    if (test_mode + gen_mode != 1) {
        std::cerr << fmt::format("kriller: \033[31merror:\033[0m: specify one "
                                 "MODE in test and generator\n");
        is_legal = false;
    }

    if (is_syntax_yacc || is_syntax) {
        parse_syntax(*input, *output, is_syntax_yacc, is_syntax, test_mode,
                     gen_mode);
    } else if (is_lexical) {
        parse_lexical(*input, *output, is_lexical, test_mode, gen_mode);
    }

    return 0;
}
