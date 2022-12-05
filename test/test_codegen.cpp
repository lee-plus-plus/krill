#include "fmt/format.h"
#include "krill/codegen.h"
#include "krill/defs.h"
#include "krill/grammar.h"
#include "krill/lexical.h"
#include "krill/regex.h"
#include "krill/syntax.h"
#include "krill/utils.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"
#include <fmt/color.h>
#include <iostream>
#include <sstream>
#include <vector>

using namespace std;
using namespace krill::type;
using namespace krill::grammar;
using namespace krill::regex;
using namespace krill::utils;
using namespace krill::codegen;
using namespace krill::runtime;

void genLexical() {
    cerr << "input multiple regexs, end with ^d\n";
    vector<string> regexs;
    while (true) {
        string line;
        getline(cin, line);
        if (cin.eof()) { break; }
        if (line.size() == 0) { continue; }
        regexs.push_back(line);
    }
    cerr << "start generate lexical parser ...\n";

    genLexicalParser(regexs, cout);
}

void genSyntax() {
    cerr << "input multiple grammar productions, end with ^d\n";
    vector<string> strs;
    while (true) {
        string line;
        getline(cin, line);
        if (cin.eof()) { break; }
        if (line.size() == 0) { continue; }
        strs.push_back(line);
    }
    cerr << "start generate syntax parser ...\n";

    Grammar grammar(strs);
    genSyntaxParser(grammar, cout);
}

void testRegex() {
    cerr << "input multiple regexs, end with empty line\n";
    vector<string> regexs;

    cin.ignore();
    int numRegexs = 0;
    for (;; numRegexs++) {
        string line;
        getline(cin, line);
        if (line.size() == 0) { break; }
        regexs.push_back(line);
    }
    cerr << "start generate lexical parser ...\n";

    for (int i = 0; i < regexs.size(); i++) {
        cerr << fmt::format("{:d}) {}\n", i, regexs[i]);
    }

    LexicalParser lexicalParser(regexs);
    cerr << "input strings: \n";
    while (true) {
        cerr << "> ";
        string line;

        getline(cin, line);
        if (cin.eof()) { break; }
        if (line.size() == 0) { continue; }

        stringstream ss;
        ss << line;
        vector<Token> tokens = lexicalParser.parseAll(ss);
        for (auto token : tokens) {
            cerr << fmt::format(
                "[{:d} \"{}\"] ", token.id,
                fmt::format(fmt::emphasis::underline, "{}", token.lval));
        }
        cerr << "\n";
    }
}

void testSyntax() {
    cerr << "input multiple grammar productions, end with empty line\n";
    vector<string> strs;

    cin.ignore();
    while (true) {
        string line;
        getline(cin, line);
        if (line.size() == 0) { break; }
        strs.push_back(line);
    }
    cerr << "start generate syntax parser ...\n";

    Grammar      grammar(strs);
    auto         actionTable = getLALR1table(grammar);
    SyntaxParser syntaxParser(grammar, actionTable);

    cerr << "terminal set of grammar:\n  ";
    for (int i : grammar.terminalSet) {
        string name = grammar.symbolNames[i];
        cerr << fmt::format("{} ", name);
    }
    cerr << "\n";

    auto symbolId = reverse(grammar.symbolNames);

    cerr << "input strings: \n";
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
        syntaxParser.printAPT(cerr);

        cerr << "\n";
    }
}

Grammar parsingYacc(istream &input) {
    vector<string> lines[3];
    // Yacc -> TokenStmts DELIM ProdStmts DELIM SubStmts
    int i = 0;
    spdlog::info("input token statements");
    while (input.good() && !input.eof()) {
        string line;
        getline(input, line);
        trim(line);
        
        if (line == "%%") {
            i++;
            if (i == 1) {
                spdlog::info("input production statements");
            } else if (i == 2) {
                spdlog::info("input additional statements");
            } else {
                spdlog::error("invalid input");
                assert(false);
            }
            continue;
        } else {
            lines[i].push_back(line);
        }
    }
    spdlog::info("end input");
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
    spdlog::info("parsing productions statements");
    while (ss.good() && !ss.eof()) {
        string w;
        ss >> w;
        if (w == "|") {
            prodSymbolStrs.push_back(currProdStr);
            spdlog::info("add prod:  {}", fmt::join(currProdStr, " "));
            currProdStr.resize(1); // reserve first elem
        } else if (w == ";") {
            prodSymbolStrs.push_back(currProdStr);
            spdlog::info("add prod:  {}", fmt::join(currProdStr, " "));
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
    spdlog::info("parsing complete");
    spdlog::debug("symbolId: {}", ToString{}(tokenIds));
    Grammar grammar(prodSymbolStrs, tokenIds, tokenPriority, tokenAssociate);
    return grammar;
}

void testYaccSyntax() {
    cerr << "input yacc definition of Ambiguous lr1 grammar, end with empty "
            "line\n";
    cerr << "format: Token_Stmts %% Production_Stmts %% Else\n";
    vector<string> strs;

    cin.ignore();
    Grammar grammar = parsingYacc(cin);

    cout << grammar.str();
    cout << "\n";

    cerr << "start generate Lr1 Automata ...\n";
    auto lr1Automata   = getLR1Automata(grammar);
    auto lalr1Automata = getLALR1fromLR1(grammar, lr1Automata);
    cout << lalr1Automata.str(grammar.symbolNames);

    cerr << "start generate Lr1 ActionTable ...\n";
    auto actionTable = getLR1table(grammar, lalr1Automata);
    cout << to_string(actionTable, grammar.symbolNames);
}

void genYaccSyntax() {
    cerr << "input yacc definition of Ambiguous lr1 grammar, end with empty "
            "line\n";
    cerr << "format: Token_Stmts %% Production_Stmts %% Else\n";
    vector<string> strs;

    cin.ignore();
    Grammar grammar = parsingYacc(cin);
    
    cerr << "start generate syntax parser ...\n";
    genSyntaxParser(grammar, cout);
}

int main() {
    cerr << "[1] syntax codegen\n"
            "[2] lexical codegen\n"
            "[3] regex test\n"
            "[4] syntax test\n"
            "[5] yacc syntax test\n"
            "[6] yacc syntax codegen\n"
            "> ";
    int i;
    cin >> i;

    if (i == 1) {
        genSyntax();
        cerr << "done!\n";
    } else if (i == 2) {
        genLexical();
        cerr << "done!\n";
    } else if (i == 3) {
        testRegex();
    } else if (i == 4) {
        testSyntax();
    } else if (i == 5) {
        testYaccSyntax();
    } else if (i == 6) {
        genYaccSyntax();
    }
    return 0;
}
