%

IDENT



program
    : decl_list
    ;

decl_list
    : decl_list decl
    | decl
    ;

decl
    : var_decl
    | fun_decl
    ;

var_decl
    : type_spec IDENT ';'
    | type_spec IDENT '[' int_literal ']' ';'
    ;

type_spec
    : VOID
    | INT
    ;

fun_decl
    : type_spec FUNCTION_IDENT '(' params ')' compound_stmt
    | type_spec FUNCTION_IDENT '(' params ')' ';'
    ;

FUNCTION_IDENT
    : IDENT
    ;

params
    : param_list
    | VOID
    ;

param_list
    : param_list ',' param
    | param
    ;

param
    : type_spec IDENT
    | type_spec IDENT '[' int_literal ']'
    ;

stmt_list
    : stmt_list stmt
    |
    ;

stmt
    : expr_stmt
    | block_stmt
    | if_stmt
    | while_stmt
    | return_stmt
    | continue_stmt
    | break_stmt
    ;


expr_stmt
    : IDENT '=' expr ';'
    | IDENT '[' expr ']' '=' expr ';'
    | '$' expr '=' expr ';'
    | IDENT '(' args_ ')' ';'
    ;

while_stmt
    : WHILE_IDENT '(' expr ')' stmt
    ;

WHILE_IDENT
    : WHILE
    ;

block_stmt
    : '{' stmt_list '}'
    ;

compound_stmt
    : '{' local_decls stmt_list '}'
    ;

local_decls
    : local_decls local_decl
    |
    ;

local_decl
    : type_spec IDENT ';'
    | type_spec IDENT '[' int_literal ']' ';'
    ;

if_stmt
    : IF '(' expr ')' stmt %prec UMINUS
    | IF '(' expr ')' stmt ELSE stmt %prec MPR
    ;

return_stmt
    : RETURN ';'
    | RETURN expr ';'
    ;


expr
    : expr OR expr
    | expr EQ expr
    | expr NE expr
    | expr LE expr
    | expr '<' expr
    | expr GE expr
    | expr '>' expr
    | expr AND expr
    | expr '+' expr
    | expr '-' expr
    | expr '*' expr
    | expr '/' expr
    | expr '%' expr
    | '!' expr %prec UMINUS
    | '-' expr %prec UMINUS
    | '+' expr %prec UMINUS
    | '$' expr %prec UMINUS
    | '(' expr ')'
    | IDENT
    | IDENT '[' expr ']'
    | IDENT '(' args_ ')'
    | int_literal
    | expr '&' expr
    | expr '^' expr
    | '~' expr
    | expr LSHIFT expr
    | expr RSHIFT expr
    | expr '|' expr
    ;


int_literal
    : DECNUM
    | HEXNUM
    ;


arg_list
    : arg_list ',' expr
    | expr
    ;


args_
    : arg_list
    |
    ;


continue_stmt
    : CONTINUE ';'
    ;

break_stmt
    : BREAK ';'
    ;


%%