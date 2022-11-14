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
        {"float", "((1|2)(0|1|2)*|0)?\\.?(0|1|2)+"},
        {"varname", "(a|b|c)(a|b|c|0|1|2)*"},
        {"oprt", "\\+|\\-|\\*|/"},
        {"'='", "="},
        {"';'", ";"},
        {"delim", " +"},
    };
    ActionTable actionTable = getLALR1table(grammar);

    SimpleLexicalParser lexicalParser(grammar, nameToRegex);
    SyntaxParser  syntaxParser(grammar, actionTable);

    string src = "a =1 + 21; b=2*0/1; 1/1-1; ";
    stringstream input;
    input << src;

    vector<Token> tokens = lexicalParser.parseAll(input);
    syntaxParser.parseAll(tokens);

    // APTnode *root = syntaxParser.getAnnotatedParsingTree();
    syntaxParser.printAnnotatedParsingTree(cout);
}

// regex-like grammar (cannot deal with escape like "\+", "\?" well)
void test2() {
    Grammar grammar({
        "RegEx -> Parallel", 
        "Parallel -> Parallel '|' Seq", 
        "Parallel -> Seq", 
        "Seq -> Seq Item", 
        "Seq -> Item ", 
        "Item -> Closure", 
        "Item -> Atom", 
        "Closure -> Atom '+'", 
        "Closure -> Atom '*'", 
        "Closure -> Atom '?'", 
        "Atom -> '(' Parallel ')'", 
        "Atom -> Char", 
        "Atom -> Range", 
        "Range -> '[' RangeSeq ']'", 
        "Range -> '[' '^' RangeSeq ']'", 
        "RangeSeq -> RangeSeq RangeItem", 
        "RangeSeq -> RangeItem", 
        "RangeItem -> Char '-' Char", 
        "RangeItem -> Char", 
    });
    map<string, string> nameToRegex = {
        {"'|'", "\\|"},
        {"'+'", "\\+"},
        {"'*'", "\\*"},
        {"'?'", "\\?"},
        {"'('", "\\("},
        {"')'", "\\)"},
        {"'['", "\\["},
        {"']'", "\\]"},
        {"'^'", "\\^"},
        {"'-'", "\\-"},
        {"Char", ".|\\+"},
    };
    ActionTable actionTable = getLALR1table(grammar);

    SimpleLexicalParser lexicalParser(grammar, nameToRegex);
    SyntaxParser  syntaxParser(grammar, actionTable);

    // string src = "a =1 + 21; b=2*0/1; 1/1-1; ";
    while (true) {
        string s;
        cout << "> input regex: ";
        cin >> s;
        stringstream input;
        input << s;

        vector<Token> tokens = lexicalParser.parseAll(input);
        syntaxParser.reset();
        syntaxParser.parseAll(tokens);

        // APTnode *root = syntaxParser.getAnnotatedParsingTree();
        syntaxParser.printAnnotatedParsingTree(cout);
    }
}

void test3() {
    Grammar grammar({
        "RegEx -> Parallel", 
        "Parallel -> Parallel '|' Seq", 
        "Parallel -> Seq", 
        "Seq -> Seq Item", 
        "Seq -> Item ", 
        "Item -> Closure", 
        "Item -> Atom", 
        "Closure -> Atom '+'", 
        "Closure -> Atom '*'", 
        "Closure -> Atom '?'", 
        "Atom -> '(' Parallel ')'", 
        "Atom -> Char", 
        "Atom -> Range", 
        "Range -> '[' RangeSeq ']'", 
        "Range -> '[' '^' RangeSeq ']'", 
        "RangeSeq -> RangeSeq RangeItem", 
        "RangeSeq -> RangeItem", 
        "RangeItem -> Char '-' Char", 
        "RangeItem -> Char", 
    });
    map<string, string> nameToRegex = {
        {"D", "[0-9]"},
        {"'|'", "\\|"},
        {"'+'", "\\+"},
        {"'*'", "\\*"},
        {"'?'", "\\?"},
        {"'('", "\\("},
        {"')'", "\\)"},
        {"'['", "\\["},
        {"']'", "\\]"},
        {"'^'", "\\^"},
        {"'-'", "\\-"},
    };
    ActionTable actionTable = getLALR1table(grammar);

    SimpleLexicalParser lexicalParser(grammar, nameToRegex);
    SyntaxParser  syntaxParser(grammar, actionTable);

    // string src = "a =1 + 21; b=2*0/1; 1/1-1; ";
    while (true) {
        string s;
        cin >> s;
        stringstream input;
        input << s;

        vector<Token> tokens = lexicalParser.parseAll(input);
        syntaxParser.reset();
        syntaxParser.parseAll(tokens);

        // APTnode *root = syntaxParser.getAnnotatedParsingTree();
        syntaxParser.printAnnotatedParsingTree(cout);
    }
}

int main() {
    vector<void (*)()> testFuncs = {test1, test2};
    for (int i = 0; i < testFuncs.size(); i++) {
        cout << "#test " << (i + 1) << endl;
        testFuncs[i]();
        cout << endl << endl;
    }
    return 0;
}
