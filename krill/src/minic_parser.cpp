#include "fmt/format.h"
#include "krill/minic.h"
#include "krill/utils.h"
#include <cassert>
#include <fmt/color.h>
#include <iostream>
#include <map>
#include <sstream>
#include <vector>
using namespace std;
using namespace krill::type;
using namespace krill::utils;
using namespace krill::runtime;
using namespace krill::minic;
using namespace krill::minic::syntax; // name of token

using krill::log::logger;

int                  col = 1;
int                  row = 1;
stringstream         source;
vector<stringstream> sourceLines = {};

void count(char c) {
    if (c == '\n') {
        row += 1;
        col = 1;
    } else if (c == '\t') {
        // col = ((col + 3) / 4) * 4 + 1; // bad
        col += 1;
        while (col % 8 != 1) { col += 1; } // good
    } else {
        col += 1;
    }
    // update source, by the way
    source << c;
    if (sourceLines.size() == 0) { sourceLines.push_back({}); }
    if (c == '\n') {
        sourceLines.push_back({});
    } else {
        sourceLines.back() << c;
    }
}

extern APTnode tokenToNode(Token token, istream &input, bool &drop) {
    APTnode node;
    node.attr.Set<string>("lval", token.lval);
    node.attr.Set<int>("col_st", col);
    node.attr.Set<int>("row_st", row);

    if (token.id == 43) { // 43: /\*
        bool match1 = false, match2 = false;
        while (input.good() && !input.eof() && !input.fail()) {
            char c = input.get();
            count(c);
            match2 = match1 && (c == '/');
            match1 = (c == '*');
            if (match2) { break; }
        }
        if (!match2) {
            throw runtime_error("error: unclosed block comment /*\n");
        }
        drop = true;
    } else if (token.id == 42) { // 42: //[^\n\r]*
        for (char c : token.lval) { count(c); }
        drop = true;
    } else if (token.id == 39 || token.id == 40) {
        // 39: [ \t]+
        // 40: \n|\r\n
        for (char c : token.lval) { count(c); }
        drop = true;
    } else { // normal token
        for (char c : token.lval) { count(c); }
        switch (token.id) {
            case -1: // END_
                node.id = -1;
                break;
            case 0: // void
                node.id = VOID;
                break;
            case 1: // continue
                node.id = CONTINUE;
                break;
            case 2: // if
                node.id = IF;
                break;
            case 3: // while
                node.id = WHILE;
                break;
            case 4: // else
                node.id = ELSE;
                break;
            case 5: // break
                node.id = BREAK;
                break;
            case 6: // int
                node.id = INT;
                break;
            case 7: // return
                node.id = RETURN;
                break;
            case 8: // \|\|
                node.id = OR;
                break;
            case 9: // &&
                node.id = AND;
                break;
            case 10: // [a-zA-Z]([a-zA-Z]|([0-9])|([1-9][0-9]*))*
                node.id = IDENT;
                break;
            case 11: // 0(x|X)([a-zA-Z]|[0-9])*
                node.id = HEXNUM;
                break;
            case 12: // ([0-9])|([1-9][0-9]*)[1-9]*
                node.id = DECNUM;
                break;
            case 13: // \<=
                node.id = LE;
                break;
            case 14: // \>=
                node.id = GE;
                break;
            case 15: // \=\=
                node.id = EQ;
                break;
            case 16: // \!\=
                node.id = NE;
                break;
            case 17: // \>
                node.id = '>';
                break;
            case 18: // \<
                node.id = '<';
                break;
            case 19: // \,
                node.id = ',';
                break;
            case 20: // \;
                node.id = ';';
                break;
            case 21: // \{
                node.id = '{';
                break;
            case 22: // \}
                node.id = '}';
                break;
            case 23: // \%
                node.id = '%';
                break;
            case 24: // \*
                node.id = '*';
                break;
            case 25: // \+
                node.id = '+';
                break;
            case 26: // \-
                node.id = '-';
                break;
            case 27: // /
                node.id = '/';
                break;
            case 28: // \=
                node.id = '=';
                break;
            case 29: // \(
                node.id = '(';
                break;
            case 30: // \)
                node.id = ')';
                break;
            case 31: // ~
                node.id = '~';
                break;
            case 32: // &
                node.id = '&';
                break;
            case 33: // \^
                node.id = '^';
                break;
            case 34: // \[
                node.id = '[';
                break;
            case 35: // \]
                node.id = ']';
                break;
            case 36: // \<\<
                node.id = LSHIFT;
                break;
            case 37: // \>\>
                node.id = RSHIFT;
                break;
            case 38: // \|
                node.id = '|';
                break;
            // case 39: // [\t ]
            //     return 0;
            //     break;
            // case 40: // \n|\r\n
            //     return 0;
            //     break;
            case 41: // $
                node.id = '$';
                break;
            // case 42: // //[^\n\r]*
            //     return 0;
            //     break;
            // case 43: // /\*
            //     return 0;
            //     break;
            case 44: // \!
                node.id = '!';
                break;
            default:
                cerr << token.id << endl;
                assert(false);
        }
        drop = false;
    }

    node.attr.Set<int>("col_ed", col);
    node.attr.Set<int>("row_ed", row);
    return node;
}

void print_located_source(int col_st, int row_st, int col_ed, int row_ed,
                          ostream &oss) {
    assert(std::tie(row_st, col_st) <= std::tie(row_ed, col_ed));
    for (int row = row_st; row <= row_ed; row++) {
        string row_str     = sourceLines[row - 1].str();
        int    curr_col_st = (row > row_st) ? 0 : (col_st - 1);
        int    curr_col_ed = (row < row_st) ? row_str.size() : (col_ed - 1);
        oss << fmt::format("{}\n{}{}\n", row_str, string(curr_col_st, ' '),
                           string(curr_col_ed - curr_col_st, '~'));
    }
}

extern void initSyntaxParser() {
    // add location attributes for every APT node
    int col = 1;
    int row = 1;

    AptNodeFunc minicActionFunc = [&col, &row](APTnode &node) {
        assert(node.attr.Has<int>("col_st"));
        assert(node.attr.Has<int>("row_st"));
        assert(node.attr.Has<int>("col_ed"));
        assert(node.attr.Has<int>("row_st"));
        col = node.attr.Get<int>("col_ed");
        row = node.attr.Get<int>("row_ed");
    };

    AptNodeFunc minicReduceFunc = [&col, &row](APTnode &node) {
        if (node.child.size() != 0) {
            node.attr.RefN<int>("col_st") =
                node.child.front().get()->attr.Get<int>("col_st");
            node.attr.RefN<int>("row_st") =
                node.child.front().get()->attr.Get<int>("row_st");
            node.attr.RefN<int>("col_ed") =
                node.child.back().get()->attr.Get<int>("col_ed");
            node.attr.RefN<int>("row_ed") =
                node.child.back().get()->attr.Get<int>("row_ed");
        } else {
            // empty production (local_decl <- )
            node.attr.RefN<int>("col_st") = col;
            node.attr.RefN<int>("row_st") = row;
            node.attr.RefN<int>("col_ed") = col;
            node.attr.RefN<int>("row_ed") = row;
        }
    };

    AptNodeFunc minicErrorFunc = [](APTnode &node) {
        int col_st = node.attr.Get<int>("col_st");
        int row_st = node.attr.Get<int>("row_st");
        int col_ed = node.attr.Get<int>("col_ed");
        int row_ed = node.attr.Get<int>("row_ed");

        string error_name = "error";

        cerr << fmt::format("filename:{}:{}: {}\n", error_name, row_st, col_st);
        print_located_source(col_st, row_st, col_ed, row_ed, cerr);
    };

    minicSyntaxParser.actionFunc_ = minicActionFunc;
    minicSyntaxParser.reduceFunc_ = minicReduceFunc;
    minicSyntaxParser.errorFunc_  = minicErrorFunc;
}