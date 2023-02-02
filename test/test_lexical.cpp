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
using krill::automata::getDFAintegrated;
using krill::regex::getDFAfromRegex;
using namespace krill::type;
using namespace krill::utils;
using namespace krill::runtime;

class SimpleLexicalParser : public LexicalParser {
  private:
    map<int, int>    toSyntaxId_; // {x, syntaxId}
    map<int, string> tokenNames_; // {syntaxId,  name}

  public:
    SimpleLexicalParser(Grammar grammar, map<string, string> nameToRegex);
    Token         parseStep(istream &input);
    vector<Token> parseAll(istream &input);
};

// to align the lexicalId and syntaxId
// nameToRegex: {terminalName, regex}
SimpleLexicalParser::SimpleLexicalParser(Grammar             grammar,
                                         map<string, string> nameToRegex)
    : LexicalParser(
          apply_map(to_vector(nameToRegex), [](pair<string, string> p) {
              return getDFAfromRegex(p.second);
          })) {

    tokenNames_ = grammar.symbolNames;
    map<int, string> lexicalNames;
    vector<DFA>      dfas;

    int i = 0;
    for (auto[name, regex] : nameToRegex) {
        lexicalNames[i] = name;
        i++;
    }

    // map<string, int> tokenNames_r = reverse(tokenNames_);
    auto tokenNames_r = reverse<int, string>(tokenNames_);
    for (auto[lexId, name] : lexicalNames) {
        if (tokenNames_r.count(name) > 0) {
            int syntaxId       = tokenNames_r.at(name);
            toSyntaxId_[lexId] = syntaxId;
        } else {
            // pass
        }
    }
    toSyntaxId_[END_SYMBOL] = END_SYMBOL;
}

// read, until one "syntax" token is generated
// lexical token which cannot map to syntax token will be dropped
// return tokens with syntax id
Token SimpleLexicalParser::parseStep(istream &input) {
    while (true) {
        Token token = LexicalParser::parseStep(input);
        if (toSyntaxId_.count(token.id) != 0) {
            token.id = toSyntaxId_.at(token.id); // lexical id => syntax id
            return token;
        } else {
            continue;
        }
    }
}

// read, until the end of input (END_TOKEN is generated)
// return tokens with syntax id
vector<Token> SimpleLexicalParser::parseAll(istream &input) {
    vector<Token> tokens;
    do {
        Token token = parseStep(input);
        tokens.push_back(token);
    } while (tokens.back() != END_TOKEN);
    return tokens;
}


void printDFA(DFA dfa, ostream &oss, bool isAscii) {
    set<int> symbolset;
    for (const auto &node : dfa.graph) {
        for (const auto &edge : node.second) { symbolset.insert(edge.first); }
    }

    oss << "DFA: \n";
    oss << "\t";
    for (int symbol : symbolset) {
        oss << fmt::format(isAscii ? "'{:c}'\t" : "_{:d}\t", symbol);
    }
    oss << "\n";
    for (const auto &elem : dfa.finality) {
        const int &state = elem.first;
        oss << fmt::format("s{}", state);
        if (dfa.finality[state] != 0) {
            oss << fmt::format("(*{:d})", dfa.finality[state]);
        }
        oss << "\t";
        for (int symbol : symbolset) {
            if (dfa.graph[state].count(symbol)) {
                oss << fmt::format("s{}\t", dfa.graph[state][symbol]);
            } else {
                oss << "\t";
            }
        }
        oss << "\n";
    }
}

void printNFA(NFA nfa, ostream &oss, bool isAscii) {
    set<int> symbolset;
    for (const auto &node : nfa.graph) {
        for (const auto &edge : node.second) { symbolset.insert(edge.first); }
    }

    oss << "NFA: \n";
    oss << "\t";
    for (int symbol : symbolset) {
        oss << fmt::format(isAscii ? "'{:c}'\t" : "_{:d}\t", symbol);
    }
    oss << "\n";
    for (const auto &elem : nfa.finality) {
        const int &state = elem.first;
        oss << fmt::format("s{}", state);
        if (nfa.finality[state] != 0) {
            oss << fmt::format("(*{:d})", nfa.finality[state]);
        }
        oss << "\t";
        for (int symbol : symbolset) {
            if (nfa.graph[state].count(symbol)) {
                auto it     = nfa.graph[state].lower_bound(symbol);
                auto it_end = nfa.graph[state].upper_bound(symbol);
                oss << fmt::format("s{}", it->second);
                for (it++; it != it_end; it++) {
                    oss << fmt::format(",{}", it->second);
                }
                oss << "\t";
            } else {
                oss << "\t";
            }
        }
        oss << "\n";
    }
}

void test1() {
    vector<string> regexStrs = {
        "(1|2)(0|1|2)*|0",       "((1|2)(0|1|2)*|0)?\\.?(0|1|2)+",
        "(a|b|c)(a|b|c|0|1|2)*", " +",
        "\\+|\\-|\\*|/",         "=",
    };
    map<int, string> symbolNames = {{-1, "END_"},   {0, "int"},   {1, "float"},
                                    {2, "varname"}, {3, "delim"}, {4, "oprt"},
                                    {5, "equal"}};

    vector<DFA> regexDFAs;
    for (string regex : regexStrs) {
        regexDFAs.push_back(getDFAfromRegex(regex));
        printDFA(regexDFAs.back(), cout, true);
    }

    DFA dfai = getDFAintegrated(regexDFAs);
    cout << "intergrated DFA:" << endl;
    printDFA(dfai, cout, true);

    LexicalParser parser(regexDFAs);

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
                                token.lval);
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
        {"float", "((1|2)(0|1|2)*|0)?\\.?(0|1|2)+"},
        {"varname", "(a|b|c)(a|b|c|0|1|2)*"},
        {"oprt", "\\+|\\-|\\*|/"},
        {"'='", "="},
        {"';'", ";"},
        {"delim", " +"},
    };

    SimpleLexicalParser parser(grammar, nameToRegex);

    vector<string> srcs = {"a =1 + 21; b=2*0/1; 1/1-1; "};

    for (string src : srcs) {
        cout << fmt::format("\n> input=\"{}\"\n", src);

        stringstream srcStream;
        srcStream << src;
        while (true) {
            Token token = parser.parseStep(srcStream);
            cout << fmt::format("[{} \"{}\"]", grammar.symbolNames[token.id],
                                token.lval);
            if (token == END_TOKEN) { break; }
        }
        cout << endl;
    }
}

int main() {
    krill::log::sink_cerr->set_level(spdlog::level::debug);
    vector<void (*)()> testFuncs = {test1, test2};
    for (int i = 0; i < testFuncs.size(); i++) {
        cout << "#test " << (i + 1) << endl;
        testFuncs[i]();
        cout << endl << endl;
    }
    return 0;
}
