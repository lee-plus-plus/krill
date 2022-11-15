#include "fmt/format.h"
#include "krill/attrdict.h"
#include "krill/defs.h"
#include "krill/grammar.h"
#include "krill/lexical.h"
#include "krill/syntax.h"
#include <cstdlib>
#include <iostream>
#include <map>
#include <sstream>
#include <vector>
using namespace std;
using namespace krill::grammar;
using namespace krill::utils;
using namespace krill::runtime;


void test1() {
    Grammar             grammar({
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
        {"varname", "[a-z][a-z0-9]*"},
        {"oprt", "\\+|\\-|\\*|/"},
        {"'='", "="},
        {"';'", ";"},
        {"delim", " +"},
    };
    ActionTable actionTable = getLALR1table(grammar);

    SimpleLexicalParser lexicalParser(grammar, nameToRegex);
    SyntaxParser        syntaxParser(grammar, actionTable);

    string       src = "a = 1 + 21; b=2*0/1; 1/1-1; ";
    stringstream input;
    input << src;

    vector<Token> tokens = lexicalParser.parseAll(input);
    syntaxParser.parseAll(tokens);

    // APTnode *root = syntaxParser.getAnnotatedParsingTree();
    syntaxParser.printAnnotatedParsingTree(cout);
}

// regex-like grammar (cannot deal with escape like "\+", "\?" well)
void test2() {
    Grammar             grammar({
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
        {"'|'", "\\|"}, {"'+'", "\\+"}, {"'*'", "\\*"},    {"'?'", "\\?"},
        {"'('", "\\("}, {"')'", "\\)"}, {"'['", "\\["},    {"']'", "\\]"},
        {"'^'", "\\^"}, {"'-'", "\\-"}, {"Char", ".|\\+"},
    };
    ActionTable actionTable = getLALR1table(grammar);

    SimpleLexicalParser lexicalParser(grammar, nameToRegex);
    SyntaxParser        syntaxParser(grammar, actionTable);

    // string src = "a =1 + 21; b=2*0/1; 1/1-1; ";
    while (true) {
        string s;
        cout << "> input regex: ";
        cin >> s;
        stringstream input;
        input << s;

        vector<Token> tokens = lexicalParser.parseAll(input);
        syntaxParser.clear();
        syntaxParser.parseAll(tokens);

        // APTnode *root = syntaxParser.getAnnotatedParsingTree();
        syntaxParser.printAnnotatedParsingTree(cout);
    }
}

void test3() {
    Grammar             grammar({
        /* 0 */ "Exp_    -> DefExps",
        /* 1 */ "DefExps -> DefExps DefExp",
        /* 2 */ "DefExps -> DefExp",
        /* 3 */ "DefExp  -> varname '=' Exp ';'",
        /* 4 */ "DefExp  -> Exp ';'",
        /* 5 */ "Exp     -> Exp oprt num",
        /* 6 */ "Exp     -> num",
        /* 7 */ "num     -> int",
        /* 8 */ "num     -> float",
    });
    map<string, string> nameToRegex = {
        {"int", "[1-9][0-9]*|0"},
        {"float", "([1-9][0-9]*|0)?\\.?[0-9]+"},
        {"varname", "[a-zA-Z][a-zA-Z0-9]*"},
        {"oprt", "\\+|\\-|\\*|/"},
        {"'='", "="},
        {"';'", ";"},
        {"delim", " +"},
    };

    ActionTable actionTable = getLALR1table(grammar);

    SimpleLexicalParser lexicalParser(grammar, nameToRegex);
    SyntaxParser        syntaxParser(grammar, actionTable);

    auto &rFunc = syntaxParser.reduceFunc_;

    // Exp_    -> DefExps
    rFunc[0] = [](AttrDict &next, deque<AttrDict> &child) { next = child[0]; };
    // DefExps -> DefExps DefExp
    rFunc[1] = [](AttrDict &next, deque<AttrDict> &child) {
        next = child[0];
        next.Ref<vector<double>>("retval_list")
            .push_back(child[1].Get<double>("val"));
    };
    // DefExps -> DefExp
    rFunc[2] = [](AttrDict &next, deque<AttrDict> &child) {
        double value = child[0].Get<double>("val");
        next.Set<vector<double>>("retval_list", {value});
        next.Set<map<string, double>>(
            "var_list", child[0].Get<map<string, double>>("var_list"));
    };
    // DefExp  -> varname '=' Exp ';'
    rFunc[3] = [](AttrDict &next, deque<AttrDict> &child) {
        string varname = child[0].Get<string>("lval");
        double value   = child[2].Get<double>("val");
        next.Set<map<string, double>>("var_list", {{varname, value}});
        next.Set<double>("val", value);
    };
    // DefExp  -> Exp ';'
    rFunc[4] = [](AttrDict &next, deque<AttrDict> &child) {
        next.Set<double>("val", child[0].Get<double>("val"));
        next.Set<map<string, double>>("var_list", {});
    };
    // Exp     -> Exp oprt num
    rFunc[5] = [](AttrDict &next, deque<AttrDict> &child) {
        double v1   = child[0].Get<double>("val");
        double v2   = child[2].Get<double>("val");
        string oprt = child[1].Get<string>("lval");
        double value;
        if (oprt == "+") {
            value = v1 + v2;
        } else if (oprt == "-") {
            value = v1 - v2;
        } else if (oprt == "*") {
            value = v1 * v2;
        } else if (oprt == "/") {
            value = v1 / v2;
        }
        next.Set<double>("val", {value});
    };
    // Exp     -> num
    rFunc[6] = [](AttrDict &next, deque<AttrDict> &child) {
        next.Set<double>("val", child[0].Get<double>("val"));
    };
    // num     -> int
    rFunc[7] = [](AttrDict &next, deque<AttrDict> &child) {
        next.Set<double>("val", atof(child[0].Get<string>("lval").c_str()));
    };
    // num     -> float
    rFunc[8] = [](AttrDict &next, deque<AttrDict> &child) {
        next.Set<double>("val", atof(child[0].Get<string>("lval").c_str()));
    };

    // string src = "a =1 + 21; b=2*0/1; 1/1-1; ";
    while (true) {
        string s;
        cout << "> input regex: ";
        getline(cin, s);
        stringstream input;
        input << s;

        vector<Token> tokens = lexicalParser.parseAll(input);
        cout << "{";
        for (auto token : tokens) {
            cout << fmt::format("[{}, \"{}\"]", token.id, token.lval);
        }
        cout << "}\n";

        syntaxParser.clear();
        syntaxParser.parseAll(tokens);

        syntaxParser.printAnnotatedParsingTree(cout);
        APTnode *root        = syntaxParser.getAnnotatedParsingTree();
        auto     var_list    = root->attr.Get<map<string, double>>("var_list");
        auto     retval_list = root->attr.Get<vector<double>>("retval_list");

        cout << "{";
        for (auto[key, value] : var_list) {
            cout << fmt::format("\"{}\": {}, ", key, value);
        }
        cout << "}\n";
        cout << fmt::format("{{{}}}\n", fmt::join(retval_list, ", "));
    }
}

int main() {
    vector<void (*)()> testFuncs = {test1, test3};
    for (int i = 0; i < testFuncs.size(); i++) {
        cout << "#test " << (i + 1) << endl;
        testFuncs[i]();
        cout << endl << endl;
    }
    return 0;
}
