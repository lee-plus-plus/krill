#include "fmt/core.h"
#include "krill/automata.h"
#include "krill/defs.h"
#include "krill/grammar.h"
#include "krill/lexical.h"
#include "krill/regex.h"
#include "krill/utils.h"
#include <iostream>
#include <map>
#include <sstream>
#include <vector>
using namespace std;
using namespace krill::automata;
using namespace krill::regex;
using namespace krill::utils;
using namespace krill::runtime;

void test1() {
    vector<string> regexStrs = {
        "(1|2)(0|1|2)*|0",       "((1|2)(0|1|2)*|0)?.?(0|1|2)+",
        "(a|b|c)(a|b|c|0|1|2)*", " +",
        "\\+|-|\\*|/",           "=",
    };
    map<int, string> symbolNames = {{-1, "END_"},   {0, "int"},   {1, "float"},
                                    {2, "varname"}, {3, "delim"}, {4, "oprt"},
                                    {5, "equal"}};

    vector<DFA> regexDFAs;
    for (string regex : regexStrs) {
        regexDFAs.push_back(getDFAfromRegex(regex));
        printDFA(regexDFAs.back(), cout, true);
    }
    BaseLexicalParser parser(regexDFAs);

    vector<string> srcs = {
        "121  abc  a21 a",
        "1.1  0.2  .2   121.0",
        "bac1   = 1.12 + 1.10",
        "c0   = 1 * 10 -2 * 2",
    };

    for (string src : srcs) {
        cout << fmt::format("\n> input=\"{}\"\n", src);

        stringstream srcStream;
        srcStream << src;
        while (true) {
            Token token = parser.parseStep(srcStream);
            cout << fmt::format("[{} \"{}\"]", symbolNames[token.id],
                                token.lexValue);
            if (token == END_TOKEN) { break; }
        }
        cout << endl;
    }
}

void test2() {
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

    LexicalParser parser(grammar, nameToRegex);

    vector<string> srcs = {"a =1 + 21; b=2*0/1; 1/1-1; "};

    for (string src : srcs) {
        cout << fmt::format("\n> input=\"{}\"\n", src);

        stringstream srcStream;
        srcStream << src;
        while (true) {
            Token token = parser.parseStep(srcStream);
            cout << fmt::format("[{} \"{}\"]", grammar.symbolNames[token.id],
                                token.lexValue);
            if (token == END_TOKEN) { break; }
        }
        cout << endl;
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
