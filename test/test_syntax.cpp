#include "fmt/format.h"
#include "krill/attrdict.h"
#include "krill/defs.h"
#include "krill/grammar.h"
#include "krill/lexical.h"
#include "krill/syntax.h"
#include "krill/utils.h"
#include <cstdlib>
#include <iostream>
#include <map>
#include <sstream>
#include <vector>
using namespace std;
using namespace krill::type;
using krill::grammar::getLALR1table;
using namespace krill::utils;
using namespace krill::runtime;

vector<string> toRegexs(map<string, string> nameToRegex) {
    vector<string> regexs;
    for (auto[name, regex] : nameToRegex) { regexs.push_back(regex); }
    return regexs;
}

// simple map lexical id to syntax id with same symbol name
map<int, int> getToSyntaxIdMap(map<int, string>    symbolNames,
                               map<string, string> nameToRegex) {
    map<int, int> toSyntaxIdMap;
    auto          symbolNames_r = reverse(symbolNames);

    int i = 0;
    for (auto[name, regex] : nameToRegex) {
        if (symbolNames_r.count(name) > 0) {
            toSyntaxIdMap[i] = symbolNames_r.at(name);
        }
        i++;
    }
    toSyntaxIdMap[END_SYMBOL] = END_SYMBOL;
    return toSyntaxIdMap;
}

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

    LexicalParser lexicalParser(toRegexs(nameToRegex));
    SyntaxParser  syntaxParser(grammar, actionTable);
    map<int, int> toSyntaxId =
        getToSyntaxIdMap(grammar.symbolNames, nameToRegex);

    string       src = "a = 1 + 21; b=2*0/1; 1/1-1; ";
    stringstream input;
    input << src;

    vector<Token> tokens = lexicalParser.parseAll(input);
    vector<Token> syntaxTokens;
    for (auto elem : tokens) {
        if (toSyntaxId.count(elem.id)) {
            elem.id = toSyntaxId.at(elem.id);
            syntaxTokens.push_back(elem);
        }
    }
    syntaxParser.clear();
    syntaxParser.parseAll(syntaxTokens);

    // APTnode *root = syntaxParser.getAnnotatedParsingTree();
    syntaxParser.printAnnotatedParsingTree(cerr);
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

    LexicalParser lexicalParser(toRegexs(nameToRegex));
    SyntaxParser  syntaxParser(grammar, actionTable);
    map<int, int> toSyntaxId =
        getToSyntaxIdMap(grammar.symbolNames, nameToRegex);

    // string src = "a =1 + 21; b=2*0/1; 1/1-1; ";
    while (true) {
        string s;
        cerr << "> input regex: ";
        cin >> s;
        stringstream input;
        input << s;

        vector<Token> tokens = lexicalParser.parseAll(input);
        vector<Token> syntaxTokens;
        for (auto elem : tokens) {
            if (toSyntaxId.count(elem.id)) {
                elem.id = toSyntaxId.at(elem.id);
                syntaxTokens.push_back(elem);
            }
        }
        syntaxParser.clear();
        syntaxParser.parseAll(syntaxTokens);

        // APTnode *root = syntaxParser.getAnnotatedParsingTree();
        syntaxParser.printAnnotatedParsingTree(cerr);
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

    LexicalParser lexicalParser(toRegexs(nameToRegex));
    SyntaxParser  syntaxParser(grammar, actionTable);
    map<int, int> toSyntaxId =
        getToSyntaxIdMap(grammar.symbolNames, nameToRegex);

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
        double value = 0.0;
        if (oprt == "+") {
            value = v1 + v2;
        } else if (oprt == "-") {
            value = v1 - v2;
        } else if (oprt == "*") {
            value = v1 * v2;
        } else if (oprt == "/") {
            value = v1 / v2;
        }
        next.Set<double>("val", value);
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
    cerr << "a very simple interpreter: \n"
            "please input codes like: \n"
            "\"a =1 + 21; b=2*0/1; 1 /1-1; \"\n";
    vector<string> inputs = {
        "a =1 + 21; b=2*0/1; 1 /1-1; ",
    };

    for (string line : inputs) {
        stringstream input;
        input << line;
        cerr << "input: " << line << "\n";
        vector<Token> tokens = lexicalParser.parseAll(input);

        cerr << "{";
        for (auto token : tokens) {
            cerr << fmt::format("[{}, \"{}\"]", token.id, token.lval);
        }
        cerr << "}\n";

        vector<Token> syntaxTokens;
        for (auto elem : tokens) {
            if (toSyntaxId.count(elem.id)) {
                elem.id = toSyntaxId.at(elem.id);
                syntaxTokens.push_back(elem);
            }
        }
        syntaxParser.clear();
        syntaxParser.parseAll(syntaxTokens);


        syntaxParser.printAnnotatedParsingTree(cerr);
        APTnode *root        = syntaxParser.getAnnotatedParsingTree();
        auto     var_list    = root->attr.Get<map<string, double>>("var_list");
        auto     retval_list = root->attr.Get<vector<double>>("retval_list");

        cerr << "variables: {";
        for (auto[key, value] : var_list) {
            cerr << fmt::format("\"{}\": {}, ", key, value);
        }
        cerr << "}\n";
        cerr << fmt::format("returns: {{{}}}\n", fmt::join(retval_list, ", "));
    }
}

int main() {
    krill::log::sink_cerr->set_level(spdlog::level::debug);
    vector<void (*)()> testFuncs = {test1, test3};
    for (int i = 0; i < testFuncs.size(); i++) {
        cerr << "#test " << (i + 1) << endl;
        testFuncs[i]();
        cerr << endl << endl;
    }
    return 0;
}
