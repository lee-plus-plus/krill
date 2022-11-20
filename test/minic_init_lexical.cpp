#include "fmt/format.h"
#include "krill/minic.h"
#include "krill/utils.h"
#include <fmt/color.h>
#include <iostream>
#include <map>
#include <sstream>
#include <cassert>
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

extern APTnode tokenToNode(Token token, istream &input, bool &drop) {
	APTnode node;
	node.attr.Set<string>("lval", token.lval);

    if (token.id == 0) { // /*
        drop = true;
        return node;
    } else if (token.id == 1) { // //[^\n\r]*
        drop = true;
        return node;
    } else if (token.id == 97) { // [ \t\v\n\r\f]+
        drop = true;
        return node;
    } else if (token.id == 98) { // .
        drop = true;
        return node;
    }
    

    switch (token.id) {
    	case -1: // END_
    		node.id = -1;
            break;
        case 2: // auto
            node.id = AUTO;
            break;
        case 3: // _Bool
            node.id = BOOL;
            break;
        case 4: // break
            node.id = BREAK;
            break;
        case 5: // case
            node.id = CASE;
            break;
        case 6: // char
            node.id = CHAR;
            break;
        case 7: // _Complex
            node.id = COMPLEX;
            break;
        case 8: // const
            node.id = CONST;
            break;
        case 9: // continue
            node.id = CONTINUE;
            break;
        case 10: // default
            node.id = DEFAULT;
            break;
        case 11: // do
            node.id = DO;
            break;
        case 12: // double
            node.id = DOUBLE;
            break;
        case 13: // else
            node.id = ELSE;
            break;
        case 14: // enum
            node.id = ENUM;
            break;
        case 15: // extern
            node.id = EXTERN;
            break;
        case 16: // float
            node.id = FLOAT;
            break;
        case 17: // for
            node.id = FOR;
            break;
        case 18: // goto
            node.id = GOTO_;
            break;
        case 19: // if
            node.id = IF;
            break;
        case 20: // _Imaginary
            node.id = IMAGINARY;
            break;
        case 21: // inline
            node.id = INLINE;
            break;
        case 22: // int
            node.id = INT;
            break;
        case 23: // long
            node.id = LONG;
            break;
        case 24: // register
            node.id = REGISTER;
            break;
        case 25: // restrict
            node.id = RESTRICT;
            break;
        case 26: // return
            node.id = RETURN;
            break;
        case 27: // short
            node.id = SHORT;
            break;
        case 28: // signed
            node.id = SIGNED;
            break;
        case 29: // sizeof
            node.id = SIZEOF;
            break;
        case 30: // static
            node.id = STATIC;
            break;
        case 31: // struct
            node.id = STRUCT;
            break;
        case 32: // switch
            node.id = SWITCH;
            break;
        case 33: // typedef
            node.id = TYPEDEF;
            break;
        case 34: // union
            node.id = UNION;
            break;
        case 35: // unsigned
            node.id = UNSIGNED;
            break;
        case 36: // void
            node.id = VOID;
            break;
        case 37: // volatile
            node.id = VOLATILE;
            break;
        case 38: // while
            node.id = WHILE;
            break;
        case 39: // [a-zA-Z_]([a-zA-Z_]|[0-9])*
            node.id = IDENTIFIER;
            break;
        case 40: // 0[xX][a-fA-F0-9]+((u|U)|(u|U)?(l|L|ll|LL)|(l|L|ll|LL)(u|U))?
            node.id = CONSTANT;
            break;
        case 41: // 0[0-7]*((u|U)|(u|U)?(l|L|ll|LL)|(l|L|ll|LL)(u|U))?
            node.id = CONSTANT;
            break;
        case 42: // [1-9][0-9]*((u|U)|(u|U)?(l|L|ll|LL)|(l|L|ll|LL)(u|U))?
            node.id = CONSTANT;
            break;
        case 43: // L?'(\\.|[^\\'\n\r])+'
            node.id = CONSTANT;
            break;
        case 44: // [0-9]+([Ee][\+\-]?[0-9]+)(f|F|l|L)?
            node.id = CONSTANT;
            break;
        case 45: // [0-9]*\.[0-9]+([Ee][\+\-]?[0-9]+)?(f|F|l|L)?
            node.id = CONSTANT;
            break;
        case 46: // [0-9]+\.[0-9]*([Ee][\+\-]?[0-9]+)?(f|F|l|L)?
            node.id = CONSTANT;
            break;
        case 47: // 0[xX][a-fA-F0-9]+([Pp][\+\-]?[0-9]+)(f|F|l|L)?
            node.id = CONSTANT;
            break;
        case 48: // 0[xX][a-fA-F0-9]*\.[a-fA-F0-9]+([Pp][\+\-]?[0-9]+)?(f|F|l|L)?
            node.id = CONSTANT;
            break;
        case 49: // 0[xX][a-fA-F0-9]+\.[a-fA-F0-9]*([Pp][\+\-]?[0-9]+)?(f|F|l|L)?
            node.id = CONSTANT;
            break;
        case 50: // L?\"(\\.|[^\\\"\n\r])*\"
            node.id = STRING_LITERAL;
            break;
        case 51: // \.\.\.
            node.id = ELLIPSIS;
            break;
        case 52: // >>=
            node.id = RIGHT_ASSIGN;
            break;
        case 53: // <<=
            node.id = LEFT_ASSIGN;
            break;
        case 54: // \+=
            node.id = ADD_ASSIGN;
            break;
        case 55: // \-=
            node.id = SUB_ASSIGN;
            break;
        case 56: // \*=
            node.id = MUL_ASSIGN;
            break;
        case 57: // /=
            node.id = DIV_ASSIGN;
            break;
        case 58: // %=
            node.id = MOD_ASSIGN;
            break;
        case 59: // &=
            node.id = AND_ASSIGN;
            break;
        case 60: // \^=
            node.id = XOR_ASSIGN;
            break;
        case 61: // \|=
            node.id = OR_ASSIGN;
            break;
        case 62: // >>
            node.id = RIGHT_OP;
            break;
        case 63: // <<
            node.id = LEFT_OP;
            break;
        case 64: // \+\+
            node.id = INC_OP;
            break;
        case 65: // \-\-
            node.id = DEC_OP;
            break;
        case 66: // \->
            node.id = PTR_OP;
            break;
        case 67: // &&
            node.id = AND_OP;
            break;
        case 68: // \|\|
            node.id = OR_OP;
            break;
        case 69: // <=
            node.id = LE_OP;
            break;
        case 70: // >=
            node.id = GE_OP;
            break;
        case 71: // ==
            node.id = EQ_OP;
            break;
        case 72: // !=
            node.id = NE_OP;
            break;
        case 73: // ;
            node.id = ';';
            break;
        case 74: // ({|<%)
            node.id = '{';
            break;
        case 75: // (}|%>)
            node.id = '}';
            break;
        case 76: // ,
            node.id = ',';
            break;
        case 77: // :
            node.id = ':';
            break;
        case 78: // =
            node.id = '=';
            break;
        case 79: // \(
            node.id = '(';
            break;
        case 80: // \)
            node.id = ')';
            break;
        case 81: // (\[|<:)
            node.id = '[';
            break;
        case 82: // (\]|:>)
            node.id = ']';
            break;
        case 83: // \.
            node.id = '.';
            break;
        case 84: // &
            node.id = '&';
            break;
        case 85: // !
            node.id = '!';
            break;
        case 86: // ~
            node.id = '~';
            break;
        case 87: // \-
            node.id = '-';
            break;
        case 88: // \+
            node.id = '+';
            break;
        case 89: // \*
            node.id = '*';
            break;
        case 90: // /
            node.id = '/';
            break;
        case 91: // %
            node.id = '%';
            break;
        case 92: // <
            node.id = '<';
            break;
        case 93: // >
            node.id = '>';
            break;
        case 94: // \^
            node.id = '^';
            break;
        case 95: // \|
            node.id = '|';
            break;
        case 96: // \?
            node.id = '?';
            break;
        default:
            assert(false);
    }
    drop = false;

    return node;
}