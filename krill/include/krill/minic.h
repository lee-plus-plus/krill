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
#define cpp_file 256
#define translation_unit 257
#define primary_expression 258
#define IDENTIFIER 259
#define CONSTANT 260
#define STRING_LITERAL 261
#define expression 262
#define postfix_expression 263
#define argument_expression_list 264
#define PTR_OP 265
#define INC_OP 266
#define DEC_OP 267
#define type_name 268
#define initializer_list 269
#define assignment_expression 270
#define unary_expression 271
#define unary_operator 272
#define cast_expression 273
#define SIZEOF 274
#define multiplicative_expression 275
#define additive_expression 276
#define shift_expression 277
#define LEFT_OP 278
#define RIGHT_OP 279
#define relational_expression 280
#define LE_OP 281
#define GE_OP 282
#define equality_expression 283
#define EQ_OP 284
#define NE_OP 285
#define and_expression 286
#define exclusive_or_expression 287
#define inclusive_or_expression 288
#define logical_and_expression 289
#define AND_OP 290
#define logical_or_expression 291
#define OR_OP 292
#define conditional_expression 293
#define assignment_operator 294
#define MUL_ASSIGN 295
#define DIV_ASSIGN 296
#define MOD_ASSIGN 297
#define ADD_ASSIGN 298
#define SUB_ASSIGN 299
#define LEFT_ASSIGN 300
#define RIGHT_ASSIGN 301
#define AND_ASSIGN 302
#define XOR_ASSIGN 303
#define OR_ASSIGN 304
#define constant_expression 305
#define declaration 306
#define declaration_specifiers 307
#define init_declarator_list 308
#define storage_class_specifier 309
#define type_specifier 310
#define type_qualifier 311
#define function_specifier 312
#define init_declarator 313
#define declarator 314
#define initializer 315
#define TYPEDEF 316
#define EXTERN 317
#define STATIC 318
#define AUTO 319
#define REGISTER 320
#define VOID 321
#define CHAR 322
#define SHORT 323
#define INT 324
#define LONG 325
#define FLOAT 326
#define DOUBLE 327
#define SIGNED 328
#define UNSIGNED 329
#define BOOL 330
#define COMPLEX 331
#define IMAGINARY 332
#define struct_or_union_specifier 333
#define enum_specifier 334
#define TYPE_NAME 335
#define struct_or_union 336
#define struct_declaration_list 337
#define STRUCT 338
#define UNION 339
#define struct_declaration 340
#define specifier_qualifier_list 341
#define struct_declarator_list 342
#define struct_declarator 343
#define ENUM 344
#define enumerator_list 345
#define enumerator 346
#define CONST 347
#define RESTRICT 348
#define VOLATILE 349
#define INLINE 350
#define pointer 351
#define direct_declarator 352
#define type_qualifier_list 353
#define parameter_type_list 354
#define identifier_list 355
#define parameter_list 356
#define ELLIPSIS 357
#define parameter_declaration 358
#define abstract_declarator 359
#define direct_abstract_declarator 360
#define designation 361
#define designator_list 362
#define designator 363
#define statement 364
#define labeled_statement 365
#define compound_statement 366
#define expression_statement 367
#define selection_statement 368
#define iteration_statement 369
#define jump_statement 370
#define CASE 371
#define DEFAULT 372
#define block_item_list 373
#define block_item 374
#define IF 375
#define ELSE 376
#define SWITCH 377
#define WHILE 378
#define DO 379
#define FOR 380
#define GOTO_ 381
#define CONTINUE 382
#define BREAK 383
#define RETURN 384
#define external_declaration 385
#define function_definition 386
#define declaration_list 387

} // namespace krill::minic::syntax

#endif