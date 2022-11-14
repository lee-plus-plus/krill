#include "fmt/format.h"
#include "krill/defs.h"
#include "krill/grammar.h"
#include "krill/lexical.h"
#include "krill/syntax.h"
#include <iostream>
#include <sstream>
#include <map>
#include <vector>
using namespace std;
using namespace krill::grammar;
using namespace krill::utils;
using namespace krill::runtime;


void test1() {
    Grammar grammar({
        "Exp_    -> DefExps",
        "DefExps -> DefExps DefExp",
        "DefExps -> DefExp",
        "DefExp  -> varname '=' Exp ';'",
        "DefExp  -> Exp ';'",
        "Exp     -> Exp oprt num",
        "Exp     -> num",
        "num     -> int",
        "num     -> float",
    });
    map<string, string> nameToRegex = {
        {"int", "(1|2)(0|1|2)*|0"},
        {"float", "((1|2)(0|1|2)*|0)?.?(0|1|2)+"},
        {"varname", "(a|b|c)(a|b|c|0|1|2)*"},
        {"oprt", "\\+|-|\\*|/"},
        {"'='", "="},
        {"';'", ";"},
        {"delim", " +"},
    };
    ActionTable actionTable = getLALR1table(grammar);

    LexicalParser lexicalParser(grammar, nameToRegex);
    SyntaxParser  syntaxParser(grammar, actionTable);

    string src = "a =1 + 21; b=2*0/1; 1/1-1; ";
    stringstream input;
    input << src;

    vector<Token> tokens = lexicalParser.parseAll(input);
    syntaxParser.parseAll(tokens);

    // APTnode *root = syntaxParser.getAnnotatedParsingTree();
    syntaxParser.printAnnotatedParsingTree(cout);

}

int main() {
    vector<void (*)()> testFuncs = {test1};
    for (int i = 0; i < testFuncs.size(); i++) {
        cout << "#test " << (i + 1) << endl;
        testFuncs[i]();
        cout << endl << endl;
    }
    return 0;
}
