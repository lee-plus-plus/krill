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

// afunc_ = defaultActionFunc;
// rfunc_0 = defaultReduceFunc;

// syntax ACTION
Afunc afunc_ = [](AttrDict &next, const Token &token) {
    next.Set<string>("lval", token.lval); // DIY
};

// 0: cpp_file -> translation_unit
Rfunc rfunc_0 = defaultReduceFunc;

// 1: primary_expression -> IDENTIFIER
Rfunc rfunc_1 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 2: primary_expression -> CONSTANT
Rfunc rfunc_2 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 3: primary_expression -> STRING_LITERAL
Rfunc rfunc_3 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 4: primary_expression -> '(' expression ')'
Rfunc rfunc_4 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 5: postfix_expression -> primary_expression
Rfunc rfunc_5 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 6: postfix_expression -> postfix_expression '[' expression ']'
Rfunc rfunc_6 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 7: postfix_expression -> postfix_expression '(' ')'
Rfunc rfunc_7 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 8: postfix_expression -> postfix_expression '(' argument_expression_list ')'
Rfunc rfunc_8 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 9: postfix_expression -> postfix_expression '.' IDENTIFIER
Rfunc rfunc_9 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 10: postfix_expression -> postfix_expression PTR_OP IDENTIFIER
Rfunc rfunc_10 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 11: postfix_expression -> postfix_expression INC_OP
Rfunc rfunc_11 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 12: postfix_expression -> postfix_expression DEC_OP
Rfunc rfunc_12 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 13: postfix_expression -> '(' type_name ')' '{' initializer_list '}'
Rfunc rfunc_13 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 14: postfix_expression -> '(' type_name ')' '{' initializer_list ',' '}'
Rfunc rfunc_14 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 15: argument_expression_list -> assignment_expression
Rfunc rfunc_15 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 16: argument_expression_list -> argument_expression_list ','
// assignment_expression
Rfunc rfunc_16 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 17: unary_expression -> postfix_expression
Rfunc rfunc_17 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 18: unary_expression -> INC_OP unary_expression
Rfunc rfunc_18 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 19: unary_expression -> DEC_OP unary_expression
Rfunc rfunc_19 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 20: unary_expression -> unary_operator cast_expression
Rfunc rfunc_20 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 21: unary_expression -> SIZEOF unary_expression
Rfunc rfunc_21 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 22: unary_expression -> SIZEOF '(' type_name ')'
Rfunc rfunc_22 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 23: unary_operator -> '&'
Rfunc rfunc_23 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 24: unary_operator -> '*'
Rfunc rfunc_24 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 25: unary_operator -> '+'
Rfunc rfunc_25 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 26: unary_operator -> '-'
Rfunc rfunc_26 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 27: unary_operator -> '~'
Rfunc rfunc_27 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 28: unary_operator -> '!'
Rfunc rfunc_28 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 29: cast_expression -> unary_expression
Rfunc rfunc_29 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 30: cast_expression -> '(' type_name ')' cast_expression
Rfunc rfunc_30 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 31: multiplicative_expression -> cast_expression
Rfunc rfunc_31 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 32: multiplicative_expression -> multiplicative_expression '*'
// cast_expression
Rfunc rfunc_32 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 33: multiplicative_expression -> multiplicative_expression '/'
// cast_expression
Rfunc rfunc_33 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 34: multiplicative_expression -> multiplicative_expression '%'
// cast_expression
Rfunc rfunc_34 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 35: additive_expression -> multiplicative_expression
Rfunc rfunc_35 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 36: additive_expression -> additive_expression '+' multiplicative_expression
Rfunc rfunc_36 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 37: additive_expression -> additive_expression '-' multiplicative_expression
Rfunc rfunc_37 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 38: shift_expression -> additive_expression
Rfunc rfunc_38 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 39: shift_expression -> shift_expression LEFT_OP additive_expression
Rfunc rfunc_39 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 40: shift_expression -> shift_expression RIGHT_OP additive_expression
Rfunc rfunc_40 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 41: relational_expression -> shift_expression
Rfunc rfunc_41 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 42: relational_expression -> relational_expression '<' shift_expression
Rfunc rfunc_42 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 43: relational_expression -> relational_expression '>' shift_expression
Rfunc rfunc_43 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 44: relational_expression -> relational_expression LE_OP shift_expression
Rfunc rfunc_44 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 45: relational_expression -> relational_expression GE_OP shift_expression
Rfunc rfunc_45 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 46: equality_expression -> relational_expression
Rfunc rfunc_46 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 47: equality_expression -> equality_expression EQ_OP relational_expression
Rfunc rfunc_47 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 48: equality_expression -> equality_expression NE_OP relational_expression
Rfunc rfunc_48 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 49: and_expression -> equality_expression
Rfunc rfunc_49 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 50: and_expression -> and_expression '&' equality_expression
Rfunc rfunc_50 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 51: exclusive_or_expression -> and_expression
Rfunc rfunc_51 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 52: exclusive_or_expression -> exclusive_or_expression '^' and_expression
Rfunc rfunc_52 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 53: inclusive_or_expression -> exclusive_or_expression
Rfunc rfunc_53 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 54: inclusive_or_expression -> inclusive_or_expression '|'
// exclusive_or_expression
Rfunc rfunc_54 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 55: logical_and_expression -> inclusive_or_expression
Rfunc rfunc_55 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 56: logical_and_expression -> logical_and_expression AND_OP
// inclusive_or_expression
Rfunc rfunc_56 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 57: logical_or_expression -> logical_and_expression
Rfunc rfunc_57 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 58: logical_or_expression -> logical_or_expression OR_OP
// logical_and_expression
Rfunc rfunc_58 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 59: conditional_expression -> logical_or_expression
Rfunc rfunc_59 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 60: conditional_expression -> logical_or_expression '?' expression ':'
// conditional_expression
Rfunc rfunc_60 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 61: assignment_expression -> conditional_expression
Rfunc rfunc_61 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 62: assignment_expression -> unary_expression assignment_operator
// assignment_expression
Rfunc rfunc_62 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 63: assignment_operator -> '='
Rfunc rfunc_63 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 64: assignment_operator -> MUL_ASSIGN
Rfunc rfunc_64 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 65: assignment_operator -> DIV_ASSIGN
Rfunc rfunc_65 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 66: assignment_operator -> MOD_ASSIGN
Rfunc rfunc_66 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 67: assignment_operator -> ADD_ASSIGN
Rfunc rfunc_67 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 68: assignment_operator -> SUB_ASSIGN
Rfunc rfunc_68 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 69: assignment_operator -> LEFT_ASSIGN
Rfunc rfunc_69 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 70: assignment_operator -> RIGHT_ASSIGN
Rfunc rfunc_70 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 71: assignment_operator -> AND_ASSIGN
Rfunc rfunc_71 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 72: assignment_operator -> XOR_ASSIGN
Rfunc rfunc_72 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 73: assignment_operator -> OR_ASSIGN
Rfunc rfunc_73 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 74: expression -> assignment_expression
Rfunc rfunc_74 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 75: expression -> expression ',' assignment_expression
Rfunc rfunc_75 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 76: constant_expression -> conditional_expression
Rfunc rfunc_76 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 77: declaration -> declaration_specifiers ';'
Rfunc rfunc_77 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 78: declaration -> declaration_specifiers init_declarator_list ';'
Rfunc rfunc_78 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 79: declaration_specifiers -> storage_class_specifier
Rfunc rfunc_79 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 80: declaration_specifiers -> storage_class_specifier declaration_specifiers
Rfunc rfunc_80 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 81: declaration_specifiers -> type_specifier
Rfunc rfunc_81 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 82: declaration_specifiers -> type_specifier declaration_specifiers
Rfunc rfunc_82 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 83: declaration_specifiers -> type_qualifier
Rfunc rfunc_83 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 84: declaration_specifiers -> type_qualifier declaration_specifiers
Rfunc rfunc_84 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 85: declaration_specifiers -> function_specifier
Rfunc rfunc_85 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 86: declaration_specifiers -> function_specifier declaration_specifiers
Rfunc rfunc_86 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 87: init_declarator_list -> init_declarator
Rfunc rfunc_87 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 88: init_declarator_list -> init_declarator_list ',' init_declarator
Rfunc rfunc_88 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 89: init_declarator -> declarator
Rfunc rfunc_89 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 90: init_declarator -> declarator '=' initializer
Rfunc rfunc_90 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 91: storage_class_specifier -> TYPEDEF
Rfunc rfunc_91 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 92: storage_class_specifier -> EXTERN
Rfunc rfunc_92 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 93: storage_class_specifier -> STATIC
Rfunc rfunc_93 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 94: storage_class_specifier -> AUTO
Rfunc rfunc_94 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 95: storage_class_specifier -> REGISTER
Rfunc rfunc_95 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 96: type_specifier -> VOID
Rfunc rfunc_96 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 97: type_specifier -> CHAR
Rfunc rfunc_97 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 98: type_specifier -> SHORT
Rfunc rfunc_98 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 99: type_specifier -> INT
Rfunc rfunc_99 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 100: type_specifier -> LONG
Rfunc rfunc_100 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 101: type_specifier -> FLOAT
Rfunc rfunc_101 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 102: type_specifier -> DOUBLE
Rfunc rfunc_102 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 103: type_specifier -> SIGNED
Rfunc rfunc_103 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 104: type_specifier -> UNSIGNED
Rfunc rfunc_104 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 105: type_specifier -> BOOL
Rfunc rfunc_105 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 106: type_specifier -> COMPLEX
Rfunc rfunc_106 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 107: type_specifier -> IMAGINARY
Rfunc rfunc_107 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 108: type_specifier -> struct_or_union_specifier
Rfunc rfunc_108 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 109: type_specifier -> enum_specifier
Rfunc rfunc_109 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 110: type_specifier -> TYPE_NAME
Rfunc rfunc_110 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 111: struct_or_union_specifier -> struct_or_union IDENTIFIER '{'
// struct_declaration_list '}'
Rfunc rfunc_111 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 112: struct_or_union_specifier -> struct_or_union '{' struct_declaration_list
// '}'
Rfunc rfunc_112 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 113: struct_or_union_specifier -> struct_or_union IDENTIFIER
Rfunc rfunc_113 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 114: struct_or_union -> STRUCT
Rfunc rfunc_114 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 115: struct_or_union -> UNION
Rfunc rfunc_115 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 116: struct_declaration_list -> struct_declaration
Rfunc rfunc_116 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 117: struct_declaration_list -> struct_declaration_list struct_declaration
Rfunc rfunc_117 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 118: struct_declaration -> specifier_qualifier_list struct_declarator_list
// ';'
Rfunc rfunc_118 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 119: specifier_qualifier_list -> type_specifier specifier_qualifier_list
Rfunc rfunc_119 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 120: specifier_qualifier_list -> type_specifier
Rfunc rfunc_120 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 121: specifier_qualifier_list -> type_qualifier specifier_qualifier_list
Rfunc rfunc_121 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 122: specifier_qualifier_list -> type_qualifier
Rfunc rfunc_122 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 123: struct_declarator_list -> struct_declarator
Rfunc rfunc_123 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 124: struct_declarator_list -> struct_declarator_list ',' struct_declarator
Rfunc rfunc_124 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 125: struct_declarator -> declarator
Rfunc rfunc_125 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 126: struct_declarator -> ':' constant_expression
Rfunc rfunc_126 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 127: struct_declarator -> declarator ':' constant_expression
Rfunc rfunc_127 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 128: enum_specifier -> ENUM '{' enumerator_list '}'
Rfunc rfunc_128 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 129: enum_specifier -> ENUM IDENTIFIER '{' enumerator_list '}'
Rfunc rfunc_129 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 130: enum_specifier -> ENUM '{' enumerator_list ',' '}'
Rfunc rfunc_130 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 131: enum_specifier -> ENUM IDENTIFIER '{' enumerator_list ',' '}'
Rfunc rfunc_131 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 132: enum_specifier -> ENUM IDENTIFIER
Rfunc rfunc_132 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 133: enumerator_list -> enumerator
Rfunc rfunc_133 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 134: enumerator_list -> enumerator_list ',' enumerator
Rfunc rfunc_134 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 135: enumerator -> IDENTIFIER
Rfunc rfunc_135 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 136: enumerator -> IDENTIFIER '=' constant_expression
Rfunc rfunc_136 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 137: type_qualifier -> CONST
Rfunc rfunc_137 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 138: type_qualifier -> RESTRICT
Rfunc rfunc_138 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 139: type_qualifier -> VOLATILE
Rfunc rfunc_139 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 140: function_specifier -> INLINE
Rfunc rfunc_140 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 141: declarator -> pointer direct_declarator
Rfunc rfunc_141 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 142: declarator -> direct_declarator
Rfunc rfunc_142 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 143: direct_declarator -> IDENTIFIER
Rfunc rfunc_143 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 144: direct_declarator -> '(' declarator ')'
Rfunc rfunc_144 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 145: direct_declarator -> direct_declarator '[' type_qualifier_list
// assignment_expression ']'
Rfunc rfunc_145 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 146: direct_declarator -> direct_declarator '[' type_qualifier_list ']'
Rfunc rfunc_146 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 147: direct_declarator -> direct_declarator '[' assignment_expression ']'
Rfunc rfunc_147 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 148: direct_declarator -> direct_declarator '[' STATIC type_qualifier_list
// assignment_expression ']'
Rfunc rfunc_148 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 149: direct_declarator -> direct_declarator '[' type_qualifier_list STATIC
// assignment_expression ']'
Rfunc rfunc_149 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 150: direct_declarator -> direct_declarator '[' type_qualifier_list '*' ']'
Rfunc rfunc_150 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 151: direct_declarator -> direct_declarator '[' '*' ']'
Rfunc rfunc_151 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 152: direct_declarator -> direct_declarator '[' ']'
Rfunc rfunc_152 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 153: direct_declarator -> direct_declarator '(' parameter_type_list ')'
Rfunc rfunc_153 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 154: direct_declarator -> direct_declarator '(' identifier_list ')'
Rfunc rfunc_154 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 155: direct_declarator -> direct_declarator '(' ')'
Rfunc rfunc_155 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 156: pointer -> '*'
Rfunc rfunc_156 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 157: pointer -> '*' type_qualifier_list
Rfunc rfunc_157 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 158: pointer -> '*' pointer
Rfunc rfunc_158 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 159: pointer -> '*' type_qualifier_list pointer
Rfunc rfunc_159 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 160: type_qualifier_list -> type_qualifier
Rfunc rfunc_160 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 161: type_qualifier_list -> type_qualifier_list type_qualifier
Rfunc rfunc_161 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 162: parameter_type_list -> parameter_list
Rfunc rfunc_162 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 163: parameter_type_list -> parameter_list ',' ELLIPSIS
Rfunc rfunc_163 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 164: parameter_list -> parameter_declaration
Rfunc rfunc_164 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 165: parameter_list -> parameter_list ',' parameter_declaration
Rfunc rfunc_165 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 166: parameter_declaration -> declaration_specifiers declarator
Rfunc rfunc_166 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 167: parameter_declaration -> declaration_specifiers abstract_declarator
Rfunc rfunc_167 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 168: parameter_declaration -> declaration_specifiers
Rfunc rfunc_168 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 169: identifier_list -> IDENTIFIER
Rfunc rfunc_169 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 170: identifier_list -> identifier_list ',' IDENTIFIER
Rfunc rfunc_170 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 171: type_name -> specifier_qualifier_list
Rfunc rfunc_171 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 172: type_name -> specifier_qualifier_list abstract_declarator
Rfunc rfunc_172 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 173: abstract_declarator -> pointer
Rfunc rfunc_173 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 174: abstract_declarator -> direct_abstract_declarator
Rfunc rfunc_174 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 175: abstract_declarator -> pointer direct_abstract_declarator
Rfunc rfunc_175 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 176: direct_abstract_declarator -> '(' abstract_declarator ')'
Rfunc rfunc_176 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 177: direct_abstract_declarator -> '[' ']'
Rfunc rfunc_177 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 178: direct_abstract_declarator -> '[' assignment_expression ']'
Rfunc rfunc_178 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 179: direct_abstract_declarator -> direct_abstract_declarator '[' ']'
Rfunc rfunc_179 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 180: direct_abstract_declarator -> direct_abstract_declarator '['
// assignment_expression ']'
Rfunc rfunc_180 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 181: direct_abstract_declarator -> '[' '*' ']'
Rfunc rfunc_181 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 182: direct_abstract_declarator -> direct_abstract_declarator '[' '*' ']'
Rfunc rfunc_182 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 183: direct_abstract_declarator -> '(' ')'
Rfunc rfunc_183 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 184: direct_abstract_declarator -> '(' parameter_type_list ')'
Rfunc rfunc_184 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 185: direct_abstract_declarator -> direct_abstract_declarator '(' ')'
Rfunc rfunc_185 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 186: direct_abstract_declarator -> direct_abstract_declarator '('
// parameter_type_list ')'
Rfunc rfunc_186 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 187: initializer -> assignment_expression
Rfunc rfunc_187 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 188: initializer -> '{' initializer_list '}'
Rfunc rfunc_188 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 189: initializer -> '{' initializer_list ',' '}'
Rfunc rfunc_189 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 190: initializer_list -> initializer
Rfunc rfunc_190 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 191: initializer_list -> designation initializer
Rfunc rfunc_191 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 192: initializer_list -> initializer_list ',' initializer
Rfunc rfunc_192 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 193: initializer_list -> initializer_list ',' designation initializer
Rfunc rfunc_193 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 194: designation -> designator_list '='
Rfunc rfunc_194 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 195: designator_list -> designator
Rfunc rfunc_195 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 196: designator_list -> designator_list designator
Rfunc rfunc_196 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 197: designator -> '[' constant_expression ']'
Rfunc rfunc_197 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 198: designator -> '.' IDENTIFIER
Rfunc rfunc_198 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 199: statement -> labeled_statement
Rfunc rfunc_199 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 200: statement -> compound_statement
Rfunc rfunc_200 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 201: statement -> expression_statement
Rfunc rfunc_201 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 202: statement -> selection_statement
Rfunc rfunc_202 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 203: statement -> iteration_statement
Rfunc rfunc_203 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 204: statement -> jump_statement
Rfunc rfunc_204 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 205: labeled_statement -> IDENTIFIER ':' statement
Rfunc rfunc_205 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 206: labeled_statement -> CASE constant_expression ':' statement
Rfunc rfunc_206 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 207: labeled_statement -> DEFAULT ':' statement
Rfunc rfunc_207 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 208: compound_statement -> '{' '}'
Rfunc rfunc_208 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 209: compound_statement -> '{' block_item_list '}'
Rfunc rfunc_209 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 210: block_item_list -> block_item
Rfunc rfunc_210 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 211: block_item_list -> block_item_list block_item
Rfunc rfunc_211 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 212: block_item -> declaration
Rfunc rfunc_212 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 213: block_item -> statement
Rfunc rfunc_213 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 214: expression_statement -> ';'
Rfunc rfunc_214 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 215: expression_statement -> expression ';'
Rfunc rfunc_215 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 216: selection_statement -> IF '(' expression ')' statement
Rfunc rfunc_216 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 217: selection_statement -> IF '(' expression ')' statement ELSE statement
Rfunc rfunc_217 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 218: selection_statement -> SWITCH '(' expression ')' statement
Rfunc rfunc_218 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 219: iteration_statement -> WHILE '(' expression ')' statement
Rfunc rfunc_219 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 220: iteration_statement -> DO statement WHILE '(' expression ')' ';'
Rfunc rfunc_220 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 221: iteration_statement -> FOR '(' expression_statement expression_statement
// ')' statement
Rfunc rfunc_221 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 222: iteration_statement -> FOR '(' expression_statement expression_statement
// expression ')' statement
Rfunc rfunc_222 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 223: iteration_statement -> FOR '(' declaration expression_statement ')'
// statement
Rfunc rfunc_223 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 224: iteration_statement -> FOR '(' declaration expression_statement
// expression ')' statement
Rfunc rfunc_224 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 225: jump_statement -> GOTO IDENTIFIER ';'
Rfunc rfunc_225 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 226: jump_statement -> CONTINUE ';'
Rfunc rfunc_226 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 227: jump_statement -> BREAK ';'
Rfunc rfunc_227 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 228: jump_statement -> RETURN ';'
Rfunc rfunc_228 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 229: jump_statement -> RETURN expression ';'
Rfunc rfunc_229 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 230: translation_unit -> external_declaration
Rfunc rfunc_230 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 231: translation_unit -> translation_unit external_declaration
Rfunc rfunc_231 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 232: external_declaration -> function_definition
Rfunc rfunc_232 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 233: external_declaration -> declaration
Rfunc rfunc_233 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 234: function_definition -> declaration_specifiers declarator
// declaration_list compound_statement
Rfunc rfunc_234 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 235: function_definition -> declaration_specifiers declarator
// compound_statement
Rfunc rfunc_235 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 236: declaration_list -> declaration
Rfunc rfunc_236 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

// 237: declaration_list -> declaration_list declaration
Rfunc rfunc_237 = [](AttrDict &next, deque<AttrDict> &child) {
    // DIY
};

void initSyntaxParser() {
    vector<Rfunc> r_funcs_ = {
        rfunc_0,   rfunc_1,   rfunc_2,   rfunc_3,   rfunc_4,   rfunc_5,
        rfunc_6,   rfunc_7,   rfunc_8,   rfunc_9,   rfunc_10,  rfunc_11,
        rfunc_12,  rfunc_13,  rfunc_14,  rfunc_15,  rfunc_16,  rfunc_17,
        rfunc_18,  rfunc_19,  rfunc_20,  rfunc_21,  rfunc_22,  rfunc_23,
        rfunc_24,  rfunc_25,  rfunc_26,  rfunc_27,  rfunc_28,  rfunc_29,
        rfunc_30,  rfunc_31,  rfunc_32,  rfunc_33,  rfunc_34,  rfunc_35,
        rfunc_36,  rfunc_37,  rfunc_38,  rfunc_39,  rfunc_40,  rfunc_41,
        rfunc_42,  rfunc_43,  rfunc_44,  rfunc_45,  rfunc_46,  rfunc_47,
        rfunc_48,  rfunc_49,  rfunc_50,  rfunc_51,  rfunc_52,  rfunc_53,
        rfunc_54,  rfunc_55,  rfunc_56,  rfunc_57,  rfunc_58,  rfunc_59,
        rfunc_60,  rfunc_61,  rfunc_62,  rfunc_63,  rfunc_64,  rfunc_65,
        rfunc_66,  rfunc_67,  rfunc_68,  rfunc_69,  rfunc_70,  rfunc_71,
        rfunc_72,  rfunc_73,  rfunc_74,  rfunc_75,  rfunc_76,  rfunc_77,
        rfunc_78,  rfunc_79,  rfunc_80,  rfunc_81,  rfunc_82,  rfunc_83,
        rfunc_84,  rfunc_85,  rfunc_86,  rfunc_87,  rfunc_88,  rfunc_89,
        rfunc_90,  rfunc_91,  rfunc_92,  rfunc_93,  rfunc_94,  rfunc_95,
        rfunc_96,  rfunc_97,  rfunc_98,  rfunc_99,  rfunc_100, rfunc_101,
        rfunc_102, rfunc_103, rfunc_104, rfunc_105, rfunc_106, rfunc_107,
        rfunc_108, rfunc_109, rfunc_110, rfunc_111, rfunc_112, rfunc_113,
        rfunc_114, rfunc_115, rfunc_116, rfunc_117, rfunc_118, rfunc_119,
        rfunc_120, rfunc_121, rfunc_122, rfunc_123, rfunc_124, rfunc_125,
        rfunc_126, rfunc_127, rfunc_128, rfunc_129, rfunc_130, rfunc_131,
        rfunc_132, rfunc_133, rfunc_134, rfunc_135, rfunc_136, rfunc_137,
        rfunc_138, rfunc_139, rfunc_140, rfunc_141, rfunc_142, rfunc_143,
        rfunc_144, rfunc_145, rfunc_146, rfunc_147, rfunc_148, rfunc_149,
        rfunc_150, rfunc_151, rfunc_152, rfunc_153, rfunc_154, rfunc_155,
        rfunc_156, rfunc_157, rfunc_158, rfunc_159, rfunc_160, rfunc_161,
        rfunc_162, rfunc_163, rfunc_164, rfunc_165, rfunc_166, rfunc_167,
        rfunc_168, rfunc_169, rfunc_170, rfunc_171, rfunc_172, rfunc_173,
        rfunc_174, rfunc_175, rfunc_176, rfunc_177, rfunc_178, rfunc_179,
        rfunc_180, rfunc_181, rfunc_182, rfunc_183, rfunc_184, rfunc_185,
        rfunc_186, rfunc_187, rfunc_188, rfunc_189, rfunc_190, rfunc_191,
        rfunc_192, rfunc_193, rfunc_194, rfunc_195, rfunc_196, rfunc_197,
        rfunc_198, rfunc_199, rfunc_200, rfunc_201, rfunc_202, rfunc_203,
        rfunc_204, rfunc_205, rfunc_206, rfunc_207, rfunc_208, rfunc_209,
        rfunc_210, rfunc_211, rfunc_212, rfunc_213, rfunc_214, rfunc_215,
        rfunc_216, rfunc_217, rfunc_218, rfunc_219, rfunc_220, rfunc_221,
        rfunc_222, rfunc_223, rfunc_224, rfunc_225, rfunc_226, rfunc_227,
        rfunc_228, rfunc_229, rfunc_230, rfunc_231, rfunc_232, rfunc_233,
        rfunc_234, rfunc_235, rfunc_236, rfunc_237};
    // Actions
    minicSyntaxParser.actionFunc_ = afunc_;
    minicSyntaxParser.reduceFunc_ = r_funcs_;
}

// ---------------------

int getMinicSyntaxId(Token token) {
    switch (token.id) {
        //  0: /*
        case 0:
            return 0; // DIY
        //  1: //[^\n\r]*
        case 1:
            return 0; // DIY
        //  2: auto
        case 2:
            return 0; // DIY
        //  3: _Bool
        case 3:
            return 0; // DIY
        //  4: break
        case 4:
            return 0; // DIY
        //  5: case
        case 5:
            return 0; // DIY
        //  6: char
        case 6:
            return 0; // DIY
        //  7: _Complex
        case 7:
            return 0; // DIY
        //  8: const
        case 8:
            return 0; // DIY
        //  9: continue
        case 9:
            return 0; // DIY
        // 10: default
        case 10:
            return 0; // DIY
        // 11: do
        case 11:
            return 0; // DIY
        // 12: double
        case 12:
            return 0; // DIY
        // 13: else
        case 13:
            return 0; // DIY
        // 14: enum
        case 14:
            return 0; // DIY
        // 15: extern
        case 15:
            return 0; // DIY
        // 16: float
        case 16:
            return 0; // DIY
        // 17: for
        case 17:
            return 0; // DIY
        // 18: goto
        case 18:
            return 0; // DIY
        // 19: if
        case 19:
            return 0; // DIY
        // 20: _Imaginary
        case 20:
            return 0; // DIY
        // 21: inline
        case 21:
            return 0; // DIY
        // 22: int
        case 22:
            return 0; // DIY
        // 23: long
        case 23:
            return 0; // DIY
        // 24: register
        case 24:
            return 0; // DIY
        // 25: restrict
        case 25:
            return 0; // DIY
        // 26: return
        case 26:
            return 0; // DIY
        // 27: short
        case 27:
            return 0; // DIY
        // 28: signed
        case 28:
            return 0; // DIY
        // 29: sizeof
        case 29:
            return 0; // DIY
        // 30: static
        case 30:
            return 0; // DIY
        // 31: struct
        case 31:
            return 0; // DIY
        // 32: switch
        case 32:
            return 0; // DIY
        // 33: typedef
        case 33:
            return 0; // DIY
        // 34: union
        case 34:
            return 0; // DIY
        // 35: unsigned
        case 35:
            return 0; // DIY
        // 36: void
        case 36:
            return 0; // DIY
        // 37: volatile
        case 37:
            return 0; // DIY
        // 38: while
        case 38:
            return 0; // DIY
        // 39: [a-zA-Z_]([a-zA-Z_]|[0-9])*
        case 39:
            return 0; // DIY
        // 40: 0[xX][a-fA-F0-9]+((u|U)|(u|U)?(l|L|ll|LL)|(l|L|ll|LL)(u|U))?
        case 40:
            return 0; // DIY
        // 41: 0[0-7]*((u|U)|(u|U)?(l|L|ll|LL)|(l|L|ll|LL)(u|U))?
        case 41:
            return 0; // DIY
        // 42: [1-9][0-9]*((u|U)|(u|U)?(l|L|ll|LL)|(l|L|ll|LL)(u|U))?
        case 42:
            return 0; // DIY
        // 43: L?'(\\.|[^\\'\n\r])+'
        case 43:
            return 0; // DIY
        // 44: [0-9]+([Ee][\+\-]?[0-9]+)(f|F|l|L)?
        case 44:
            return 0; // DIY
        // 45: [0-9]*\.[0-9]+([Ee][\+\-]?[0-9]+)?(f|F|l|L)?
        case 45:
            return 0; // DIY
        // 46: [0-9]+\.[0-9]*([Ee][\+\-]?[0-9]+)?(f|F|l|L)?
        case 46:
            return 0; // DIY
        // 47: 0[xX][a-fA-F0-9]+([Pp][\+\-]?[0-9]+)(f|F|l|L)?
        case 47:
            return 0; // DIY
        // 48: 0[xX][a-fA-F0-9]*\.[a-fA-F0-9]+([Pp][\+\-]?[0-9]+)?(f|F|l|L)?
        case 48:
            return 0; // DIY
        // 49: 0[xX][a-fA-F0-9]+\.[a-fA-F0-9]*([Pp][\+\-]?[0-9]+)?(f|F|l|L)?
        case 49:
            return 0; // DIY
        // 50: L?\"(\\.|[^\\\"\n\r])*\"
        case 50:
            return 0; // DIY
        // 51: \.\.\.
        case 51:
            return 0; // DIY
        // 52: >>=
        case 52:
            return 0; // DIY
        // 53: <<=
        case 53:
            return 0; // DIY
        // 54: \+=
        case 54:
            return 0; // DIY
        // 55: \-=
        case 55:
            return 0; // DIY
        // 56: \*=
        case 56:
            return 0; // DIY
        // 57: /=
        case 57:
            return 0; // DIY
        // 58: %=
        case 58:
            return 0; // DIY
        // 59: &=
        case 59:
            return 0; // DIY
        // 60: \^=
        case 60:
            return 0; // DIY
        // 61: \|=
        case 61:
            return 0; // DIY
        // 62: >>
        case 62:
            return 0; // DIY
        // 63: <<
        case 63:
            return 0; // DIY
        // 64: \+\+
        case 64:
            return 0; // DIY
        // 65: \-\-
        case 65:
            return 0; // DIY
        // 66: \->
        case 66:
            return 0; // DIY
        // 67: &&
        case 67:
            return 0; // DIY
        // 68: \|\|
        case 68:
            return 0; // DIY
        // 69: <=
        case 69:
            return 0; // DIY
        // 70: >=
        case 70:
            return 0; // DIY
        // 71: ==
        case 71:
            return 0; // DIY
        // 72: !=
        case 72:
            return 0; // DIY
        // 73: ;
        case 73:
            return 0; // DIY
        // 74: ({|<%)
        case 74:
            return 0; // DIY
        // 75: (}|%>)
        case 75:
            return 0; // DIY
        // 76: ,
        case 76:
            return 0; // DIY
        // 77: :
        case 77:
            return 0; // DIY
        // 78: =
        case 78:
            return 0; // DIY
        // 79: \(
        case 79:
            return 0; // DIY
        // 80: \)
        case 80:
            return 0; // DIY
        // 81: (\[|<:)
        case 81:
            return 0; // DIY
        // 82: (\]|:>)
        case 82:
            return 0; // DIY
        // 83: \.
        case 83:
            return 0; // DIY
        // 84: &
        case 84:
            return 0; // DIY
        // 85: !
        case 85:
            return 0; // DIY
        // 86: ~
        case 86:
            return 0; // DIY
        // 87: \-
        case 87:
            return 0; // DIY
        // 88: \+
        case 88:
            return 0; // DIY
        // 89: \*
        case 89:
            return 0; // DIY
        // 90: /
        case 90:
            return 0; // DIY
        // 91: %
        case 91:
            return 0; // DIY
        // 92: <
        case 92:
            return 0; // DIY
        // 93: >
        case 93:
            return 0; // DIY
        // 94: \^
        case 94:
            return 0; // DIY
        // 95: \|
        case 95:
            return 0; // DIY
        // 96: \?
        case 96:
            return 0; // DIY
        // 97: [ \t\v\n\r\f]+
        case 97:
            return 0; // DIY
        // 98: .
        case 98:
            return 0; // DIY
        default:
            assert(false);
    }
}

// ---------------------

void testLexicalParsing() {
    auto &lexicalParser = minicLexicalParser;
    cerr << "input characters (end with ^d): \n"
            "e.g., int main() {\\n} \n";

    vector<Token> tokens = lexicalParser.parseAll(cin);
    for (auto token : tokens) {
        cerr << fmt::format(
            "[{:d} \"{}\"] ", token.id,
            fmt::format(fmt::emphasis::underline, "{}", unescape(token.lval)));
    }
    cerr << "\n";
}

void testSyntaxParsing() {
    Grammar &grammar = minicGrammar;
    SyntaxParser &syntaxParser = minicSyntaxParser;

    cerr << "terminal set of grammar:\n  ";
    for (int i : grammar.terminalSet) {
        string name = grammar.symbolNames[i];
        cerr << fmt::format("{} ", name);
    }
    cerr << "\n";

    auto symbolId = reverse(grammar.symbolNames);

    cerr << "input terminals sequence (end with END_ and enter): \n"
            "e.g., INT IDENTIFIER '(' '}' '{' '}' END_\n";
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
                    fmt::format(fmt::emphasis::underline, name));
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

int main(int argc, char **argv) {
    initSyntaxParser();
    // testLexicalParsing();
    cerr << "[1] lexical test\n"
            "[2] syntax test\n"
            "> ";
    int i;
    cin >> i;
    cin.ignore();

    if (i == 1) {
        testLexicalParsing();
        cerr << "done!\n";
    } else if (i == 2) {
        testSyntaxParsing();
        cerr << "done!\n";
    } 
    return 0;
}
