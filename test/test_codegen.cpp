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
        syntaxParser.printAnnotatedParsingTree(cerr);

        cerr << "\n";
    }
}

Grammar parsingYacc(istream &input) {
    vector<string> lines[3];

    // Yacc -> TokenStmts DELIM ProdStmts DELIM SubStmts
    int i = 0;
    while (input.good() && !input.eof()) {
        string line;
        getline(input, line);
        trim(line);
        lines[i].push_back(line);
        if (line == "%%") { i++; }
    }
    assert(i < 3);

    vector<string> &tokenStmts = lines[0];
    vector<string> &prodStmts  = lines[1];
    // vector<string> &subStmts = lines[2];

    // token statement processing
    int priority    = -1;
    using Associate = Grammar::Associate;
    map<string, Associate> wordsAssociate;
    map<string, int>       wordsPriority;
    string                 startWord;

    for (string line : tokenStmts) {
        vector<string> words = split(line, " ");
        if (words.size() == 0) {
            continue;
            spdlog::info("look {}", words[0]);
        } else if (words[0] == "%token") {
            // drop line
        } else if (words[0] == "%left") {
            for (int j = 1; j < words.size(); j++) {
                assert(wordsAssociate.count(words[j]) == 0);
                assert(wordsPriority.count(words[j]) == 0);
                wordsAssociate[words[j]] = Associate::kLeft;
                wordsPriority[words[j]]  = priority;
                spdlog::info("add Left Associate Token:  {}", words[j]);
            }
            priority -= 1;
        } else if (words[0] == "%right") {
            for (int j = 1; j < words.size(); j++) {
                assert(wordsAssociate.count(words[j]) == 0);
                assert(wordsPriority.count(words[j]) == 0);
                wordsAssociate[words[j]] = Associate::kRight;
                wordsPriority[words[j]]  = priority;
                spdlog::info("add Right Associate Token:  {}", words[j]);
            }
            priority -= 1;
        } else if (words[0] == "%nonassoc") {
            for (int j = 1; j < words.size(); j++) {
                assert(wordsPriority.count(words[j]) == 0);
                wordsPriority[words[j]] = priority;
                spdlog::info("add Non Associate Token:  {}", words[j]);
            }
            priority -= 1;
        } else if (words[0] == "%start") {
            assert(words.size() == 2);
            spdlog::info("add Start Token:  {}", words[1]);
            startWord = words[1];
        }
    }

    // productions statement processing
    vector<vector<string>> prodSymbolStrs;
    map<int, int>          prodsPrior;
    map<int, Associate>    prodsAsso;

    stringstream ss;
    for (string line : prodStmts) { ss << line << " "; }


    vector<string> currProdStr;
    bool           flagPrec = false;
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
            continue; // week format check
        } else if (w[0] == '/' && w[1] == '*') {
            while (!(w[w.size() - 2] == '*' && w[w.size() - 1] == '/')) {
                ss >> w; // ignore comment
                if (!(ss.good() && !ss.eof())) { break; }
            }
        } else if (w == "%prec") {
            flagPrec = true;
        } else {
            static string assoName[] = {"None", "Left", "Right"};
            if (flagPrec) {
                int  p     = prodSymbolStrs.size();
                int  prior = wordsPriority.at(w);
                auto asso  = wordsAssociate.at(w);
                flagPrec   = false;

                prodsPrior[p]  = prior;
                prodsAsso[p] = asso;
                spdlog::info("set priority:  {}", prior);
                spdlog::info("set associativity:  {}",
                             assoName[static_cast<int>(asso)]);
            } else {
                if (wordsAssociate.count(w) > 0) {
                    int  p    = prodSymbolStrs.size();
                    auto asso = wordsAssociate.at(w);
                    prodsAsso[p] = asso;
                    spdlog::info("set associativity:  {}",
                                 assoName[static_cast<int>(asso)]);
                }
                currProdStr.push_back(w);
            }
        }
    }

    // SubStmts processing
    // pass

    // summurize
    Grammar grammar(prodSymbolStrs);
    for (auto[p, prior] : prodsPrior) {
        grammar.prodsPriority[p] = prior;
    }
    for (auto[p, asso] : prodsAsso) {
        grammar.prodsAssociate[p] = asso;
    }

    return grammar;
}

void printLR1State(LR1State state, const map<int, string> &symbolNames,
                   ostream &oss) {
    for (const auto &item : state) {
        oss << "  " << item.str(symbolNames) << "\n";
    }
}

void printEdgeTable(EdgeTable edgeTable, const map<int, string> &symbolNames,
                    ostream &oss) {
    oss << "EdgeTable: \n";
    for (const Edge &edge : edgeTable) {
        oss << fmt::format("s{:<2d} --> {:s} --> s{:<2d}\n", edge.from,
                           symbolNames.at(edge.symbol), edge.to);
    }
}

void printLr1Automata(LR1Automata             lr1Automata,
                      const map<int, string> &symbolNames, ostream &oss) {
    int i = 0;
    for (LR1State state : lr1Automata.states) {
        cout << fmt::format("{}): \n", i++);
        printLR1State(state, symbolNames, oss);
    }
    printEdgeTable(lr1Automata.edgeTable, symbolNames, oss);
}

void printActionTable(const ActionTable &     actionTable,
                      const map<int, string> &symbolNames, ostream &oss) {
    oss << fmt::format("Analysis Table (size={}): \n", actionTable.size());
    string typeName[] = {"ACTION", "REDUCE", "GOTO  ", "ACCEPT"};
    for (auto[key, action] : actionTable) {
        oss << fmt::format("s{:<2d} --> {:s} --> {:<6s} ", key.first,
                           symbolNames.at(key.second), typeName[action.type]);
        if (action.type == ACTION || action.type == GOTO) {
            oss << fmt::format("s{:<2d}", action.tgt);
        } else if (action.type == REDUCE) {
            oss << fmt::format("r{:<2d}", action.tgt);
        }
        oss << "\n";
    }
}



void testAmbiguousSyntax() {
    cerr << "input yacc definition of ambiguous lr1 grammar, end with empty "
            "line\n";
    vector<string> strs;

    cin.ignore();
    Grammar grammar = parsingYacc(cin);
    cerr << "start generate Grammar ...\n";

    cout << grammar.str();
    cout << "\n";

    cerr << "start generate Lr1 Automata ...\n";
    auto lr1Automata   = getLR1Automata(grammar);
    auto lalr1Automata = getLALR1fromLR1(grammar, lr1Automata);
    printLr1Automata(lalr1Automata, grammar.symbolNames, cout);

    cerr << "start generate Lr1 ActionTable ...\n";
    auto actionTable = getLR1table(grammar, lalr1Automata);
    printActionTable(actionTable, grammar.symbolNames, cout);
    // SyntaxParser syntaxParser(grammar, actionTable);
}

int main() {
    cerr << "[1] syntax codegen\n"
            "[2] lexical codegen\n"
            "[3] regex test\n"
            "[4] syntax test\n"
            "[5] ambiguous syntax test\n"
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
        testAmbiguousSyntax();
    }
    return 0;
}
