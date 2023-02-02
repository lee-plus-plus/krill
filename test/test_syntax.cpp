#include "fmt/format.h"
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
#include <functional>
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

    Lexer lexicalParser(toRegexs(nameToRegex));
    Parser  syntaxParser(grammar, actionTable);
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

    shared_ptr<AstNode> root = syntaxParser.getAstRoot();
    cerr << AstPrinter{}.print(root.get()) << "\n";
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
        {"'^'", "\\^"}, {"'-'", "\\-"}, 
        {"Char", "[^\\|\\+\\*\\?\\(\\)\\[\\]\\^\\-]"},
    };
    ActionTable actionTable = getLALR1table(grammar);

    Lexer lexicalParser(toRegexs(nameToRegex));
    Parser  syntaxParser(grammar, actionTable);
    map<int, int> toSyntaxId =
        getToSyntaxIdMap(grammar.symbolNames, nameToRegex);

    // string src = "a =1 + 21; b=2*0/1; 1/1-1; ";
    vector<string> src = {"abc", "ab+c*d", "a|bc+(ed+f)", "[^ab]"};
    for (string s : src) {
        cerr << "input regex: " << s << endl;
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
    }
}

// void test3() {
//     Grammar             grammar({
//         /* 0 */ "Exp_    -> DefExps",
//         /* 1 */ "DefExps -> DefExps DefExp",
//         /* 2 */ "DefExps -> DefExp",
//         /* 3 */ "DefExp  -> varname '=' Exp ';'",
//         /* 4 */ "DefExp  -> Exp ';'",
//         /* 5 */ "Exp     -> Exp oprt num",
//         /* 6 */ "Exp     -> num",
//         /* 7 */ "num     -> int",
//         /* 8 */ "num     -> float",
//     });
//     map<string, string> nameToRegex = {
//         {"int", "[1-9][0-9]*|0"},
//         {"float", "([1-9][0-9]*|0)?\\.?[0-9]+"},
//         {"varname", "[a-zA-Z][a-zA-Z0-9]*"},
//         {"oprt", "\\+|\\-|\\*|/"},
//         {"'='", "="},
//         {"';'", ";"},
//         {"delim", " +"},
//     };

//     ActionTable actionTable = getLALR1table(grammar);

//     Lexer lexicalParser(toRegexs(nameToRegex));
//     Parser  syntaxParser(grammar, actionTable);
//     map<int, int> toSyntaxId =
//         getToSyntaxIdMap(grammar.symbolNames, nameToRegex);

//     // syntax-directed-translation action
//     auto sdt_action = [&](shared_ptr<AstNode> &node, int dot) -> void {
//         // auto &id = node.get()->id; // unused
//         auto &pidx  = node.get()->pidx;

//         AttrDict *next  = &node.get()->attr;
//         deque<AttrDict *> child;
//         for (auto &c : node.get()->child) {
//             c.get()->attr.Set<string>("lval", c.get()->lval);
//             child.push_back(&c.get()->attr);
//         }

//         using PairSD = pair<string, double>;

//         if (dot == grammar.prods[pidx].right.size()) {
//             switch (pidx) {
//                 case 0: { // Exp_    -> DefExps
//                     *next = *child[0];
//                     break;
//                 }
//                 case 1: { // DefExps -> DefExps DefExp
//                     *next = *child[0];
//                     next->Ref<vector<double>>("retval_list")
//                         .push_back(child[1]->Get<double>("val"));
//                     next->Set<vector<PairSD>>(
//                         "var_list", child[0]->Get<vector<PairSD>>("var_list"));
//                     auto &list = next->Ref<vector<PairSD>>("var_list");
//                     auto &list2 = child[1]->Ref<vector<PairSD>>("var_list");
//                     list.insert(list.end(), list2.begin(), list2.end());
//                     break;
//                 }
//                 case 2: { // DefExps -> DefExp
//                     double value = child[0]->Get<double>("val");
//                     next->Set<vector<double>>("retval_list", {value});
//                     next->Set<vector<PairSD>>(
//                         "var_list", child[0]->Get<vector<PairSD>>("var_list"));
//                     break;
//                 }
//                 case 3: { // DefExp  -> varname '=' Exp ';'
//                     string varname = child[0]->Get<string>("lval");
//                     double value   = child[2]->Get<double>("val");
//                     next->Set<vector<PairSD>>("var_list",
//                                                   {{varname, value}});
//                     next->Set<double>("val", value);
//                     break;
//                 }
//                 case 4: { // DefExp  -> Exp ';'
//                     next->Set<double>("val", child[0]->Get<double>("val"));
//                     next->Set<vector<PairSD>>("var_list", {});
//                     break;
//                 }
//                 case 5: { // Exp     -> Exp oprt num
//                     double v1    = child[0]->Get<double>("val");
//                     double v2    = child[2]->Get<double>("val");
//                     string oprt  = child[1]->Get<string>("lval");
//                     double value = 0.0;
//                     if (oprt == "+") {
//                         value = v1 + v2;
//                     } else if (oprt == "-") {
//                         value = v1 - v2;
//                     } else if (oprt == "*") {
//                         value = v1 * v2;
//                     } else if (oprt == "/") {
//                         value = v1 / v2;
//                     }
//                     next->Set<double>("val", value);
//                     break;
//                 }
//                 case 6: { // Exp     -> num
//                     next->Set<double>("val", child[0]->Get<double>("val"));
//                     break;
//                 }
//                 case 7: { // num     -> int
//                     next->Set<double>(
//                         "val", atof(child[0]->Get<string>("lval").c_str()));
//                     break;
//                 }
//                 case 8: { // num     -> float
//                     next->Set<double>(
//                         "val", atof(child[0]->Get<string>("lval").c_str()));
//                     break;
//                 }
//                 default:
//                     assert(false);
//                     break;
//             }
//         }
//     };
//     // syntax-directed-translation: left-to-right visit on AST
//     std::function<void (shared_ptr<AstNode> &node)> sdt;
//     sdt = [&](shared_ptr<AstNode> &node) -> void {
//         int i;
//         for (i = 0; i < node.get()->child.size(); i++) {
//             sdt_action(node, i);
//             sdt(node.get()->child[i]);
//         }
//         sdt_action(node, i);
//     };

//     // string src = "a =1 + 21; b=2*0/1; 1/1-1; ";
//     cerr << "a very simple interpreter: \n"
//             "accept input codes like: \n"
//             "\"a =1 + 21; b=2*0/1; 1 /1-1; \"\n";
//     vector<string> inputs = {
//         "a =1 + 21; b=2*0/1; 1 /1-1; ",
//     };

//     for (string line : inputs) {
//         stringstream input;
//         input << line;
//         cerr << "input: " << line << "\n";
//         vector<Token> tokens = lexicalParser.parseAll(input);

//         cerr << "{";
//         for (auto token : tokens) {
//             cerr << fmt::format("[{}, \"{}\"]", token.id, token.lval);
//         }
//         cerr << "}\n";

//         vector<Token> syntaxTokens;
//         for (auto elem : tokens) {
//             if (toSyntaxId.count(elem.id)) {
//                 elem.id = toSyntaxId.at(elem.id);
//                 syntaxTokens.push_back(elem);
//             }
//         }
//         syntaxParser.clear();
//         syntaxParser.parseAll(syntaxTokens);

//         // syntax-directed translation
//         auto root = syntaxParser.getAstRoot();
//         sdt(root);

//         // print APT
//         cerr << AstPrinter{}.showAttrs().print(root.get()) << "\n";

//         // print translated result
//         auto var_list    = root.get()->attr.Get<vector<pair<string, double>>>("var_list");
//         auto retval_list = root.get()->attr.Get<vector<double>>("retval_list");

//         cerr << "variables: {";
//         for (auto[key, value] : var_list) {
//             cerr << fmt::format("\"{}\": {}, ", key, value);
//         }
//         cerr << "}\n";
//         cerr << fmt::format("returns: {{{}}}\n", fmt::join(retval_list, ", "));
//     }
// }

int main() {
    krill::log::sink_cerr->set_level(spdlog::level::debug);
    vector<void (*)()> testFuncs = {test1 , test2 };
    for (int i = 0; i < testFuncs.size(); i++) {
        cerr << "#test " << (i + 1) << endl;
        testFuncs[i]();
        cerr << endl << endl;
    }
    return 0;
}
