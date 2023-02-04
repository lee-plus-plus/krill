#include "fmt/core.h"
#include "krill/automata.h"
#include "krill/defs.h"
#include "krill/grammar.h"
#include "krill/regex.h"
#include "krill/utils.h"
#include <doctest/doctest.h>
#include <iostream>
#include <sstream>
#include <vector>
using namespace std;
using namespace krill::type;
using namespace krill::automata;
using namespace krill::grammar;
using namespace krill::regex;
using namespace krill::regex::core;
using namespace krill::utils;

// "abc",
// "a?b+c*d",
// "b(ac?a|b)+d",
// "(1|2)(0|1|2)*|0",
// "[0-2]",
// "[^a-y]",
// "[^a-zA-Z0-9]",
// "\\n",

TEST_CASE("regex_01") {
    try {
        DFA dfa1 = getDFAfromRegex("abc");
        CHECK(match(dfa1, "abc"));
        CHECK(unmatch(dfa1, "ab"));
        CHECK(unmatch(dfa1, "abb"));
        CHECK(unmatch(dfa1, "acb"));

        DFA dfa2 = getDFAfromRegex("a?b+c*d");
        CHECK(match(dfa2, "abcd"));
        CHECK(match(dfa2, "bcd"));
        CHECK(match(dfa2, "bbbbcccccccd"));
        CHECK(match(dfa2, "bbbbd"));
        CHECK(match(dfa2, "abbbbd"));
        CHECK(unmatch(dfa2, "abbbb"));
        CHECK(unmatch(dfa2, "ad"));
        CHECK(unmatch(dfa2, "aabcd"));
        CHECK(unmatch(dfa2, "acd"));

        DFA dfa3 = getDFAfromRegex("b(ac?a|b)+d");
        CHECK(match(dfa3, "bacad"));
        CHECK(match(dfa3, "bbd"));
        CHECK(match(dfa3, "bacaaabd"));
        CHECK(match(dfa3, "bbd"));
        CHECK(match(dfa3, "bbacabacad"));
        CHECK(unmatch(dfa3, "bbacabacacad"));
        CHECK(unmatch(dfa3, "bacaad"));

        DFA dfa4 = getDFAfromRegex("(1|2)(0|1|2)*|0");
        CHECK(match(dfa4, "0"));
        CHECK(match(dfa4, "2"));
        CHECK(match(dfa4, "10"));
        CHECK(match(dfa4, "2012"));
        CHECK(unmatch(dfa4, "01"));
        CHECK(unmatch(dfa4, "021"));

        DFA dfa5 = getDFAfromRegex("[0-2]");
        CHECK(match(dfa5, "0"));
        CHECK(match(dfa5, "1"));
        CHECK(match(dfa5, "2"));
        CHECK(unmatch(dfa5, "01"));
        CHECK(unmatch(dfa5, "3"));
        CHECK(unmatch(dfa5, "a"));
        CHECK(unmatch(dfa5, "\t"));

        DFA dfa6 = getDFAfromRegex("[^a-y]");
        CHECK(match(dfa6, "z"));
        CHECK(match(dfa6, "A"));
        CHECK(match(dfa6, "Z"));
        CHECK(match(dfa6, "0"));
        CHECK(match(dfa6, "9"));
        CHECK(match(dfa6, "."));
        CHECK(match(dfa6, " "));
        CHECK(match(dfa6, "\t"));
        CHECK(match(dfa6, "\n"));
        CHECK(match(dfa6, "\r"));
        CHECK(unmatch(dfa6, "a"));
        CHECK(unmatch(dfa6, "d"));
        CHECK(unmatch(dfa6, "x"));
        CHECK(unmatch(dfa6, "y"));

        DFA dfa7 = getDFAfromRegex("[^a-zA-Z0-9]");
        CHECK(match(dfa7, "."));
        CHECK(match(dfa7, ";"));
        CHECK(match(dfa7, "]"));
        CHECK(match(dfa7, "*"));
        CHECK(match(dfa7, " "));
        CHECK(match(dfa7, "\t"));
        CHECK(match(dfa7, "\n"));
        CHECK(match(dfa7, "\r"));
        CHECK(unmatch(dfa7, "b"));
        CHECK(unmatch(dfa7, "z"));
        CHECK(unmatch(dfa7, "B"));
        CHECK(unmatch(dfa7, "Z"));
        CHECK(unmatch(dfa7, "1"));
        CHECK(unmatch(dfa7, "8"));

        DFA dfa8 = getDFAfromRegex("\\n");
        CHECK(match(dfa8, "\n"));
        CHECK(unmatch(dfa8, "\\n"));

    } catch (std::runtime_error &err) { CHECK(false); }
}
