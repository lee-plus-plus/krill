#include "fmt/format.h"
#include "krill/lexical.h"
#include "krill/automata.h"
#include "krill/regex.h"
#include "krill/utils.h"
#include <cassert>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
using namespace krill::type;
using namespace std;
using krill::automata::getDFAintegrated;
using krill::regex::getDFAfromRegex;


namespace krill::type {

const Token END_TOKEN = Token({.id = END_SYMBOL, .lval = ""});

bool Token::operator<(const Token &t) const {
    return std::tie(id, lval) < std::tie(t.id, t.lval);
}

bool Token::operator==(const Token &t) const {
    return std::tie(id, lval) == std::tie(t.id, t.lval);
}

bool Token::operator!=(const Token &t) const {
    return std::tie(id, lval) != std::tie(t.id, t.lval);
}
} // namespace krill::type


namespace krill::runtime {

LexicalParser::LexicalParser(DFA dfai) : state_(0), dfa_(dfai) {}

LexicalParser::LexicalParser(vector<DFA> dfas)
    : state_(0), dfa_(getDFAintegrated(dfas)) {}

LexicalParser::LexicalParser(vector<string> regexs) {
    state_ = 0;
    vector<DFA> dfas;
    for (string regex : regexs) { dfas.push_back(getDFAfromRegex(regex)); }
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
                throw runtime_error(
                    fmt::format("Lexical Error: unmatched \"{}\" in \"{}\"\n",
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
        if (history_.size() > 20) { history_ = history_.substr(10); }
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

} // namespace krill::runtime