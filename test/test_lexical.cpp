#include "fmt/core.h"
#include "krill/automata.h"
#include "krill/defs.h"
#include "krill/grammar.h"
#include "krill/lexical.h"
#include "krill/regex.h"
#include "krill/utils.h"
#include <doctest/doctest.h>
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

TEST_CASE("lexer_01") {
    try {
        vector<string> regexStrs = {
            "(1|2)(0|1|2)*|0",                //  INT
            "((1|2)(0|1|2)*|0)?\\.?(0|1|2)+", // FLOAT
            "(a|b|c)(a|b|c|0|1|2)*",          // IDENT
            " +",                             // DELIM
            "\\+|\\-|\\*|/",                  // OPRT
            "=",                              // EQ
        };
        Lexer lexer(regexStrs);

        vector<string> srcs = {
            "121  abc  a21 a",
            "1.1  0.2  .2   121.0",
            "bac1   = 1.12 + 1.10",
            "c0   = 1 * 10 -2 * 2",
        };

        for (string src : srcs) {
            lexer.clear();

            stringstream srcStream;
            srcStream << src;

            string lval;
            while (true) {
                Token token = lexer.parseStep(srcStream);
                lval += token.lval;
                if (token == END_TOKEN) { break; }
            }

            CHECK(lval == src);
        }

    } catch (exception &err) { CHECK(false); }
}

TEST_CASE("lexer_02") {
    try {
        vector<string> regexs = {
            "(1|2)(0|1|2)*|0",                // INT
            "((1|2)(0|1|2)*|0)?\\.?(0|1|2)+", // FLOAT
            "(a|b|c)(a|b|c|0|1|2)*",          // VARNAME
            "\\+|\\-|\\*|/",                  // OPRT
            "=",                              // EQ
            ";",                              // SEMICOLON
            " +",                             // DELIM
        };
        constexpr int END_      = -1;
        constexpr int INT       = 0;
        constexpr int FLOAT     = 1;
        constexpr int VARNAME   = 2;
        constexpr int OPRT      = 3;
        constexpr int EQ        = 4;
        constexpr int SEMICOLON = 5;
        constexpr int DELIM     = 6;

        Lexer lexer(regexs);

        string       src = "a =1 + 21; b=2*0/1; 1/1.021-1; ";
        stringstream srcStream;
        srcStream << src;

        vector<int> tokenIds;
        while (true) {
            Token token = lexer.parseStep(srcStream);
            tokenIds.push_back(token.id);
            if (token == END_TOKEN) { break; }
        }

        CHECK(tokenIds ==
              vector<int>{
                  VARNAME, DELIM,     EQ,    INT,       DELIM, OPRT, DELIM,
                  INT,     SEMICOLON, DELIM, VARNAME,   EQ,    INT,  OPRT,
                  INT,     OPRT,      INT,   SEMICOLON, DELIM, INT,  OPRT,
                  FLOAT,   OPRT,      INT,   SEMICOLON, DELIM, END_});
    } catch (exception &err) { CHECK(false); }
}