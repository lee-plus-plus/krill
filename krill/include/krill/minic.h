#ifndef MINIC_H
#define MINIC_H
#include "defs.h"
#include "lexical.h"
#include "syntax.h"
#include <deque>
#include <memory>
#include <ostream>
#include <stack>
#include <string>
using krill::runtime::SyntaxParser, krill::runtime::LexicalParser;
using krill::type::Grammar;

namespace krill::minic {

extern Grammar       minicGrammar;
extern SyntaxParser  minicSyntaxParser;
extern LexicalParser minicLexicalParser;
extern int           getMinicSyntaxId(Token token);

} // namespace krill::minic

namespace krill::minic::syntax {

// Grammar
#define END_ -1
#define program 256
#define decl_list 257
#define decl 258
#define var_decl 259
#define fun_decl 260
#define type_spec 261
#define var_declarator_list 262
#define var_declarator 263
#define IDENT 264
#define int_literal 265
#define VOID 266
#define INT 267
#define FUNCTION_IDENT 268
#define params 269
#define compound_stmt 270
#define param_list 271
#define param 272
#define stmt_list 273
#define stmt 274
#define expr_stmt 275
#define block_stmt 276
#define if_stmt 277
#define while_stmt 278
#define return_stmt 279
#define continue_stmt 280
#define break_stmt 281
#define expr 282
#define args_ 283
#define WHILE_IDENT 284
#define WHILE 285
#define local_decls 286
#define local_decl 287
#define local_declarator_list 288
#define local_declarator 289
#define IF 290
#define ELSE 291
#define RETURN 292
#define logical_expr 293
#define AND 294
#define OR 295
#define bit_op_expr 296
#define relational_expr 297
#define EQ 298
#define NE 299
#define LE 300
#define GE 301
#define shift_expr 302
#define LSHIFT 303
#define add_expr 304
#define RSHIFT 305
#define mul_expr 306
#define unary_expr 307
#define primary_expr 308
#define DECNUM 309
#define HEXNUM 310
#define arg_list 311
#define CONTINUE 312
#define BREAK 313

} // namespace krill::minic::syntax

#endif