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
#define ζ -1
#define IDENT 258
#define VOID 259
#define INT 260
#define WHILE 261
#define IF 262
#define ELSE 263
#define RETURN 264
#define EQ 265
#define NE 266
#define LE 267
#define GE 268
#define AND 269
#define OR 270
#define DECNUM 271
#define CONTINUE 272
#define BREAK 273
#define HEXNUM 274
#define LSHIFT 275
#define RSHIFT 276
#define program 277
#define decl_list 278
#define decl 279
#define var_decl 280
#define fun_decl 281
#define type_spec 282
#define int_literal 283
#define FUNCTION_IDENT 284
#define params 285
#define compound_stmt 286
#define param_list 287
#define param 288
#define stmt_list 289
#define stmt 290
#define expr_stmt 291
#define block_stmt 292
#define if_stmt 293
#define while_stmt 294
#define return_stmt 295
#define continue_stmt 296
#define break_stmt 297
#define expr 298
#define args_ 299
#define WHILE_IDENT 300
#define local_decls 301
#define local_decl 302
#define arg_list 303

} // namespace krill::minic::syntax

#endif