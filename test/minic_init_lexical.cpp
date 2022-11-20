#include "fmt/format.h"
#include "krill/minic.h"
#include "krill/utils.h"
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
using namespace krill::minic::syntax;

int col = 1;
int row = 1;

void count(char c) {
	if (c == '\n') {
		row += 1;
		col = 1;
	} else {
		col += 1;
	}
}

extern pair<Token, APTnode> getSyntaxToken(Token token, istream &input, bool &drop) {
    if (token.id == -1) { // END_TOKEN
        token.id = -1;
    } else if (token.id == 0) { // /*

    } else if (token.id == 1) { // //[^\n\r]*

    } else if (token.id == 97) { // [ \t\v\n\r\f]+

    } else if (token.id == 98) { // .
    }
    isAccepted = false;
    return token;

    switch (token.id) {
        case 2: // auto
            token.id = AUTO;
            break;
        case 3: // _Bool
            token.id = BOOL;
            break;
        case 4: // break
            token.id = BREAK;
            break;
        case 5: // case
            token.id = CASE;
            break;
        case 6: // char
            token.id = CHAR;
            break;
        case 7: // _Complex
            token.id = COMPLEX;
            break;
        case 8: // const
            token.id = CONST;
            break;
        case 9: // continue
            token.id = CONTINUE;
            break;
        case 10: // default
            token.id = DEFAULT;
            break;
        case 11: // do
            token.id = DO;
            break;
        case 12: // double
            token.id = DOUBLE;
            break;
        case 13: // else
            token.id = ELSE;
            break;
        case 14: // enum
            token.id = ENUM;
            break;
        case 15: // extern
            token.id = EXTERN;
            break;
        case 16: // float
            token.id = FLOAT;
            break;
        case 17: // for
            token.id = FOR;
            break;
        case 18: // goto
            token.id = GOTO;
            break;
        case 19: // if
            token.id = IF;
            break;
        case 20: // _Imaginary
            token.id = IMAGINARY;
            break;
        case 21: // inline
            token.id = INLINE;
            break;
        case 22: // int
            token.id = INT;
            break;
        case 23: // long
            token.id = LONG;
            break;
        case 24: // register
            token.id = REGISTER;
            break;
        case 25: // restrict
            token.id = RESTRICT;
            break;
        case 26: // return
            token.id = RETURN;
            break;
        case 27: // short
            token.id = SHORT;
            break;
        case 28: // signed
            token.id = SIGNED;
            break;
        case 29: // sizeof
            token.id = SIZEOF;
            break;
        case 30: // static
            token.id = STATIC;
            break;
        case 31: // struct
            token.id = STRUCT;
            break;
        case 32: // switch
            token.id = SWITCH;
            break;
        case 33: // typedef
            token.id = TYPEDEF;
            break;
        case 34: // union
            token.id = UNION;
            break;
        case 35: // unsigned
            token.id = UNSIGNED;
            break;
        case 36: // void
            token.id = VOID;
            break;
        case 37: // volatile
            token.id = VOLATILE;
            break;
        case 38: // while
            token.id = WHILE;
            break;
        case 39: // [a-zA-Z_]([a-zA-Z_]|[0-9])*
            token.id = IDENTIFIER;
            break;
        case 40: // 0[xX][a-fA-F0-9]+((u|U)|(u|U)?(l|L|ll|LL)|(l|L|ll|LL)(u|U))?
            token.id = CONSTANT;
            break;
        case 41: // 0[0-7]*((u|U)|(u|U)?(l|L|ll|LL)|(l|L|ll|LL)(u|U))?
            token.id = CONSTANT;
            break;
        case 42: // [1-9][0-9]*((u|U)|(u|U)?(l|L|ll|LL)|(l|L|ll|LL)(u|U))?
            token.id = CONSTANT;
            break;
        case 43: // L?'(\\.|[^\\'\n\r])+'
            token.id = CONSTANT;
            break;
        case 44: // [0-9]+([Ee][\+\-]?[0-9]+)(f|F|l|L)?
            token.id = CONSTANT;
            break;
        case 45: // [0-9]*\.[0-9]+([Ee][\+\-]?[0-9]+)?(f|F|l|L)?
            token.id = CONSTANT;
            break;
        case 46: // [0-9]+\.[0-9]*([Ee][\+\-]?[0-9]+)?(f|F|l|L)?
            token.id = CONSTANT;
            break;
        case 47: // 0[xX][a-fA-F0-9]+([Pp][\+\-]?[0-9]+)(f|F|l|L)?
            token.id = CONSTANT;
            break;
        case 48: // 0[xX][a-fA-F0-9]*\.[a-fA-F0-9]+([Pp][\+\-]?[0-9]+)?(f|F|l|L)?
            token.id = CONSTANT;
            break;
        case 49: // 0[xX][a-fA-F0-9]+\.[a-fA-F0-9]*([Pp][\+\-]?[0-9]+)?(f|F|l|L)?
            token.id = CONSTANT;
            break;
        case 50: // L?\"(\\.|[^\\\"\n\r])*\"
            token.id = STRING_LITERAL;
            break;
        case 51: // \.\.\.
            token.id = ELLIPSIS;
            break;
        case 52: // >>=
            token.id = RIGHT_ASSIGN;
            break;
        case 53: // <<=
            token.id = LEFT_ASSIGN;
            break;
        case 54: // \+=
            token.id = ADD_ASSIGN;
            break;
        case 55: // \-=
            token.id = SUB_ASSIGN;
            break;
        case 56: // \*=
            token.id = MUL_ASSIGN;
            break;
        case 57: // /=
            token.id = DIV_ASSIGN;
            break;
        case 58: // %=
            token.id = MOD_ASSIGN;
            break;
        case 59: // &=
            token.id = AND_ASSIGN;
            break;
        case 60: // \^=
            token.id = XOR_ASSIGN;
            break;
        case 61: // \|=
            token.id = OR_ASSIGN;
            break;
        case 62: // >>
            token.id = RIGHT_OP;
            break;
        case 63: // <<
            token.id = LEFT_OP;
            break;
        case 64: // \+\+
            token.id = INC_OP;
            break;
        case 65: // \-\-
            token.id = DEC_OP;
            break;
        case 66: // \->
            token.id = PTR_OP;
            break;
        case 67: // &&
            token.id = AND_OP;
            break;
        case 68: // \|\|
            token.id = OR_OP;
            break;
        case 69: // <=
            token.id = LE_OP;
            break;
        case 70: // >=
            token.id = GE_OP;
            break;
        case 71: // ==
            token.id = EQ_OP;
            break;
        case 72: // !=
            token.id = NE_OP;
            break;
        case 73: // ;
            token.id = ';';
            break;
        case 74: // ({|<%)
            token.id = '{';
            break;
        case 75: // (}|%>)
            token.id = '}';
            break;
        case 76: // ,
            token.id = ',';
            break;
        case 77: // :
            token.id = ':';
            break;
        case 78: // =
            token.id = '=';
            break;
        case 79: // \(
            token.id = '(';
            break;
        case 80: // \)
            token.id = ')';
            break;
        case 81: // (\[|<:)
            token.id = '[';
            break;
        case 82: // (\]|:>)
            token.id = ']';
            break;
        case 83: // \.
            token.id = '.';
            break;
        case 84: // &
            token.id = '&';
            break;
        case 85: // !
            token.id = '!';
            break;
        case 86: // ~
            token.id = '~';
            break;
        case 87: // \-
            token.id = '-';
            break;
        case 88: // \+
            token.id = '+';
            break;
        case 89: // \*
            token.id = '*';
            break;
        case 90: // /
            token.id = '/';
            break;
        case 91: // %
            token.id = '%';
            break;
        case 92: // <
            token.id = '<';
            break;
        case 93: // >
            token.id = '>';
            break;
        case 94: // \^
            token.id = '^';
            break;
        case 95: // \|
            token.id = '|';
            break;
        case 96: // \?
            token.id = '?';
            break;
        default:
            assert;
    }
    isAccepted = true;

    return token;
}

extern Token getMinicSyntaxToken(Token token, istream &input);