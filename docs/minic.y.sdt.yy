%

// [IDENT].var
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

// [expr].var
expr
    : expr OR expr 
    | expr EQ expr
    | expr NE expr 
    | expr LE expr
    | expr '<' expr
    | expr GE expr
    | expr '>' expr 
    | expr AND expr 
    // expr : expr '+' expr
    //      [0].var <- assign_new_variable()
    //      codegen("{} = add {} {}", [0].var, [1].var, [2].var)
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
    // expr : IDENT '[' expr ']'
    //      width <- assign_new_variable()
    //      offset <- assign_new_variable()
    //      addr <- assign_new_variable()
    //      [0].var <- assign_new_variable()
    //      codegen("{} = assign {}", width, 4)
    //      codegen("{} = mult {} {}", offset, [3].var, width)
    //      codegen("{} = add {} {}", addr, [0].var, offset)
    //      codegen("{} = load {} {}", [0].var, addr, "%0")
    | IDENT '[' expr ']'
    // expr : IDENT '(' args_ ')' 
    //      assert([0].var in func_decl_table)
    //      assert([2].argc == func_decl_table.at([0].var).argc)
    //      [0].var <- assign_new_variable()
    //      codegen("{} = call {}", [0].var, IDENT.var)
    | IDENT '(' args_ ')' 
    | int_literal 
    | expr '&' expr 
    | expr '^' expr 
    | '~' expr 
    | expr LSHIFT expr 
    | expr RSHIFT expr 
    | expr '|' expr 
    ;

// [int_literal].cval
int_literal 
    : DECNUM
    // int_literal : DECNUM
    //      [0].cval <- atoi([1].lval)
    | HEXNUM
    ; 

// [arg_list].argc
arg_list
    // arg_list : arg_list ',' expr
    //      [0].argc <- [1].argc + 1
    //      codegen("push {}", [3].var)
    : arg_list ',' expr
    | expr
    ;

// [arg_list].argc
args_
    // args_ : arg_list
    //      [args_].argc <- [arg_list].argc
    : arg_list
    |
    ;

// use inherited attribute (FUCK)
continue_stmt
    // continue_stmt : CONTINUE '; 
    //      [0].
    : CONTINUE ';'
    ;

break_stmt
    : BREAK ';'
    ;


%%