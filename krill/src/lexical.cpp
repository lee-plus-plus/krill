#include "krill/lexical.h"
#include "fmt/format.h"
#include "krill/automata.h"
#include "krill/regex.h"
#include "krill/utils.h"
#include <cassert>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
using krill::error::parse_error;
using krill::log::logger;
using namespace krill::type;
using namespace std;
using krill::automata::getDFAintegrated;
using krill::regex::getDFAfromRegex;
using krill::utils::unescape, krill::utils::apply_map;

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

Lexer::Lexer(DFA dfai)
    : dfa_(dfai), state_(0), history_(""), row_curr_(1), col_curr_(1) {}

Lexer::Lexer(vector<DFA> dfas)
    : dfa_(getDFAintegrated(dfas)), state_(0), history_(""), row_curr_(1), col_curr_(1) {}

Lexer::Lexer(vector<string> regexs)
    : dfa_(getDFAintegrated(apply_map(regexs, getDFAfromRegex))), state_(0), history_(""),
      row_curr_(1), col_curr_(1) {
}

void Lexer::count(const string &word) {
    for (char c : word) {
        if (c == '\n') {
            row_curr_ += 1;
            col_curr_ = 1;
        } else if (c == '\t') {
            // col_ = ((col_ + 3) / 4) * 4 + 1; // bad
            col_curr_ += 1;
            while (col_curr_ % 8 != 1) { col_curr_ += 1; } // good
        } else {
            col_curr_ += 1;
        }
    }
}


// read, until one token is generated
// return token with lexical id
Token Lexer::parseStep(istream &input) {
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
                logger.debug("lexer: unmatched ‘{}’ in ‘{}’", buffer.str() + c,
                             unescape(history_ + c));
                throw runtime_error(fmt::format("lexer: unmatched ‘{}’ in ‘{}’",
                                                buffer.str() + c,
                                                unescape(history_ + c)));
            }
            int    tokenId       = dfa_.finality.at(state_) - 1;
            string tokenLexValue = buffer.str();
            int    tokenRowSt    = row_curr_;
            int    tokenColSt    = col_curr_;

            count(tokenLexValue); // update row_curr_, col_curr_
            int tokenRowEd = row_curr_;
            int tokenColEd = col_curr_;

            assert(tokenLexValue.size() > 0); // otherwise, dead loop

            state_ = 0;
            buffer.clear();

            Token token{.id     = tokenId,
                        .lval   = tokenLexValue,
                        .row_st = tokenRowSt,
                        .col_st = tokenColSt,
                        .row_ed = tokenRowEd,
                        .col_ed = tokenColEd};

            logger.debug("lexer: <token {}> ‘{}’ ({}:{}-{}:{})", token.id,
                         unescape(token.lval), tokenRowSt, tokenColSt,
                         tokenRowEd, tokenColEd);
            return token;
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
vector<Token> Lexer::parseAll(istream &input) {
    vector<Token> tokens;
    do {
        Token token = parseStep(input);
        tokens.push_back(token);
    } while (tokens.back() != END_TOKEN);
    return tokens;
}

void Lexer::clear() {
    state_    = 0;
    history_  = "";
    row_curr_ = 1;
    col_curr_ = 1;
}

} // namespace krill::runtime