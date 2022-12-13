#include "fmt/format.h"
#include "krill/minic.h"
#include "krill/utils.h"
#include <cstring>
#include <fmt/color.h>
#include <iostream>
#include <map>
#include <sstream>
#include <vector>
using krill::log::logger;
using namespace std;
using namespace krill::type;
using namespace krill::utils;
using namespace krill::runtime;
using namespace krill::minic;

extern APTnode tokenToNode(Token token, istream &input, bool &drop);
extern void    initSyntaxParser();
extern void    syntax_directed_translation(shared_ptr<APTnode> &node);
extern string  get_ir_str();

void testLexicalParsing() {
    auto &lexicalParser = minicLexicalParser;
    cerr << "input characters (end with ^d): \n"
            "e.g., int main() {\\n} \n";

    vector<Token> tokens = lexicalParser.parseAll(cin);
    for (auto token : tokens) {
        cout << fmt::format(
            "[{:d} \"{}\"] ", token.id,
            fmt::format(fmt::emphasis::underline, "{}", unescape(token.lval)));
    }
    cout << "\n";
}

void testSyntaxParsing() {
    Grammar &     grammar      = minicGrammar;
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

void testFullLexicalParsing() {
    auto &   lexicalParser = minicLexicalParser;
    Grammar &grammar       = minicGrammar;
    cerr << "input characters (end with ^d): \n"
            "e.g., int main() {\\n} \n";

    vector<Token>   tokens;
    vector<APTnode> nodes;
    while (true) {
        Token   token;
        APTnode node;
        bool    drop;

        token = lexicalParser.parseStep(cin);
        node  = tokenToNode(token, cin, drop);
        if (drop) { continue; }

        nodes.push_back(node);
        if (token == END_TOKEN) { break; }
    }

    for (auto node : nodes) {
        cout << fmt::format(
            "[{} \"{}\"] ", grammar.symbolNames.at(node.id),
            fmt::format(fmt::emphasis::underline, "{}",
                        unescape(node.attr.Get<string>("lval"))));
    }
    cout << "\n";
}

void testFullLexicalSyntaxParsing() {
    auto &lexicalParser = minicLexicalParser;
    auto &syntaxParser  = minicSyntaxParser;
    auto &grammar       = minicGrammar;
    cerr << "input characters (end with ^d): \n"
            "e.g., int main() {\\n} \n";

    vector<APTnode> nodes;
    while (true) {
        Token   token;
        APTnode node;
        bool    drop;

        token = lexicalParser.parseStep(cin);
        node  = tokenToNode(token, cin, drop);
        if (drop) { continue; }

        nodes.push_back(node);
        syntaxParser.parseStep(node);

        if (token == END_TOKEN) { break; }
    }

    for (auto node : nodes) {
        cout << fmt::format("{} ", grammar.symbolNames.at(node.id));
    }
    logger.debug("annotated parsing tree:\n{}", syntaxParser.getAPTstr());

    cout << "\n";
    cout << syntaxParser.getASTstr();
    cout << "\n";
}

void testIRgeneration() {
    auto &lexicalParser = minicLexicalParser;
    auto &syntaxParser  = minicSyntaxParser;
    auto &grammar       = minicGrammar;
    cerr << "input characters (end with ^d): \n"
            "e.g., int main() {\\n} \n";

    vector<APTnode> nodes;
    while (true) {
        Token   token;
        APTnode node;
        bool    drop;

        token = lexicalParser.parseStep(cin);
        node  = tokenToNode(token, cin, drop);
        if (drop) { continue; }

        nodes.push_back(node);
        syntaxParser.parseStep(node);

        if (token == END_TOKEN) { break; }
    }

    for (auto node : nodes) {
        cout << fmt::format("{} ", grammar.symbolNames.at(node.id));
    }
    logger.debug("annotated parsing tree:\n{}", syntaxParser.getAPTstr());

    cout << "\n";
    cout << syntaxParser.getAPTstr();
    cout << "\n";

    krill::log::sink_cerr->set_level(spdlog::level::debug);

    // syntax-directed translation
    auto root = syntaxParser.getAPT();
    syntax_directed_translation(root);

    // show result
    cout << get_ir_str();
    cout << getAPTstr(root, minicGrammar);

}

const char usage[] = "usage: test_minic {-l|-L|-s}\n"
                     "        -l    lexical parsing test\n"
                     "        -L    lexical parsing test (full)\n"
                     "        -s    syntax parsing test\n"
                     "        -Ls   lexical & syntax parsing test\n"
                     "        -I    intermediate code generation\n";

int main(int argc, char **argv) {
    if (argc != 2) {
        cerr << usage;
        return 1;
    }
    initSyntaxParser();

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
        testIRgeneration();
        cerr << "done!\n";
    } else {
        cerr << usage;
        return 1;
    }
    return 0;
}
