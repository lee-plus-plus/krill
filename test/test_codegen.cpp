#include "fmt/format.h"
// #include "krill/automata.h"
#include "krill/codegen.h"
#include "krill/defs.h"
#include "krill/grammar.h"
#include "krill/lexical.h"
#include "krill/regex.h"
#include "krill/syntax.h"
#include "krill/utils.h"
#include <fmt/color.h>
#include <iostream>
// #include <map>
#include <sstream>
#include <vector>
using namespace std;
using namespace krill::type;
// using namespace krill::automata;
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
        syntaxParser.printAnnotatedParsingTree(cerr);

        cerr << "\n";
    }
}

int main() {
    cerr << "[1] syntax codegen\n"
            "[2] lexical codegen\n"
            "[3] regex test\n"
            "[4] syntax test\n"
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
    }
    return 0;
}
