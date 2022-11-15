#include "krill/lexical.h"
#include "krill/automata.h"
#include "krill/grammar.h"
#include "krill/regex.h"
#include <cassert>
#include <sstream>
#include <iostream>
#include <string>
#include <stdexcept>
#include "fmt/format.h"
using krill::automata::DFA, krill::automata::getDFAintegrated;
using krill::grammar::Token, krill::grammar::END_SYMBOL, krill::grammar::END_TOKEN;
using krill::regex::getDFAfromRegex;
// using krill::utils::reverse;
using namespace std;

namespace krill::runtime {

LexicalParser::LexicalParser(DFA dfai)
    : state_(0), dfa_(dfai) {}

LexicalParser::LexicalParser(vector<DFA> dfas)
    : state_(0), dfa_(getDFAintegrated(dfas)) {}

LexicalParser::LexicalParser(vector<string> regexs) {
    state_ = 0;
    vector<DFA> dfas;
    for (string regex : regexs) {
        dfas.push_back(getDFAfromRegex(regex));
    }
    dfa_ = getDFAintegrated(dfas);
}


// read, until one token is generated
// return token with lexical id
Token LexicalParser::parseStep(istream &input) {
    // return end_token when input end
    if (!(input.good() && !input.eof() && !input.fail())) { return END_TOKEN; }

    stringstream buffer;
    while (true) {
        char c = input.get();

        // if cannot continue, try to accept token
        if (dfa_.graph.count(state_) == 0 ||
            dfa_.graph.at(state_).count(c) == 0) {
            input.putback(c);

            // assert(dfa_.finality.at(state_) != 0); // failed
            if (dfa_.finality.at(state_) == 0) {
                throw runtime_error(fmt::format(
                    "Lexical Error: unmatched \"{}\" in \"{}\"\n", 
                    buffer.str() + c, history_ + c));
            }
            int    tokenId       = dfa_.finality.at(state_) - 1;
            string tokenLexValue = buffer.str();

            state_ = 0;
            buffer.clear();

            assert(tokenLexValue.size() > 0);
            return Token({tokenId, tokenLexValue});
        }

        // continue
        state_ = dfa_.graph.at(state_).at(c);
        buffer << c;
        history_.push_back(c);
        if (history_.size() > 20) {
            history_ = history_.substr(10);
        }
    }
}

// read, until the end of input (END_TOKEN is generated)
// return tokens with lexical id
vector<Token> LexicalParser::parseAll(istream &input) {
    vector<Token> tokens;
    do {
        Token token = parseStep(input);
        tokens.push_back(token);
    } while (tokens.back() != END_TOKEN);
    return tokens;
}

template <typename T1, typename T2>
map<T2, T1> reverse(map<T1, T2> m) {
    map<T2, T1> m_reversed;
    for (auto [key, value] : m) {
        m_reversed[value] = key;
    }
    return m_reversed;
}

// to align the lexicalId and syntaxId
// nameToRegex: {terminalName, regex}
SimpleLexicalParser::SimpleLexicalParser(Grammar grammar, map<string, string> nameToRegex) {
    state_      = 0;
    tokenNames_ = grammar.symbolNames;
    map<int, string> lexicalNames;
    vector<DFA>      dfas;

    int i = 0;
    for (auto[name, regex] : nameToRegex) {
        dfas.push_back(getDFAfromRegex(regex));
        lexicalNames[i] = name;
        i++;
    }
    dfa_ = getDFAintegrated(dfas);

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
            token.id    = toSyntaxId_.at(token.id); // lexical id => syntax id
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

} // namespace krill::runtime