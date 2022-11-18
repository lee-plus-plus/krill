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

void testLexicalParsing() {
    auto &lexicalParser = minicLexicalParser;
    cerr << "input characters (end with ^d): \n"
            "e.g., int main() {\\n} \n";

    vector<Token> tokens = lexicalParser.parseAll(cin);
    for (auto token : tokens) {
        cerr << fmt::format(
            "[{:d} \"{}\"] ", token.id,
            fmt::format(fmt::emphasis::underline, "{}", unescape(token.lval)));
    }
    cerr << "\n";
}

void testSyntaxParsing() {
    Grammar &grammar = minicGrammar;
    SyntaxParser &syntaxParser = minicSyntaxParser;

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
                    fmt::format(fmt::emphasis::underline, name));
                isNameOK == false;
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

int main(int argc, char **argv) {
    // testLexicalParsing();
    cerr << "[1] lexical test\n"
            "[2] syntax test\n"
            "> ";
    int i;
    cin >> i;
    cin.ignore();

    if (i == 1) {
        testLexicalParsing();
        cerr << "done!\n";
    } else if (i == 2) {
        testSyntaxParsing();
        cerr << "done!\n";
    } 
    return 0;
}
