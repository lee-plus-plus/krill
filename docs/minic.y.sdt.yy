%

IDENT
// [IDENT].var


program
    : decl_list // 0
    ; 

decl_list
    : decl_list decl // 1
    | decl // 2
    ;

decl
    : var_decl // 3
    | fun_decl // 4
    ;

var_decl
    : type_spec IDENT ';' // 5
    | type_spec IDENT '[' int_literal ']' ';' // 6
    ; 

type_spec
    : VOID // 7
    | INT // 8
    ; 

fun_decl
    : type_spec FUNCTION_IDENT '(' params ')' compound_stmt // 9
    // func_decl <- type_spec FUNCTION_IDENT '(' params ')' compound_stmt
    //      map<Var, Lbl> [0].funcLblTbl = {}
    | type_spec FUNCTION_IDENT '(' params ')' ';' // 10
    ; 

FUNCTION_IDENT
    : IDENT // 11
    ; 

params
    : param_list // 12
    | VOID // 13
    ; 

param_list
    : param_list ',' param // 14
    | param // 15
    ;

param
    : type_spec IDENT // 16
    | type_spec IDENT '[' int_literal ']' // 17
    ;

stmt_list
    : stmt_list stmt // 18
    // stmt_list : stmt_list stmt
    //      [0].* <- [1].*
    //          [0].lbls <- [0].lbls | [line + [1].code.size() for line in [2].lbls]
    //      [0].code <- [0].code | [2].code
    | // 19
    // stmt_list : 
    //      [0].lbls <- {}
    //      [0].code <- {}
    ;

stmt
    : expr_stmt // 20
    | block_stmt // 21
    | if_stmt // 22
    | while_stmt // 23
    | return_stmt // 24
    | continue_stmt // 25
    | break_stmt // 26
    // stmt : break_stmt
    //      [0].* <- [1].*
    ;


expr_stmt
    : IDENT '=' expr ';' // 27
    | IDENT '[' expr ']' '=' expr ';' // 28
    | '$' expr '=' expr ';' // 29
    | IDENT '(' args_ ')' ';' // 30
    ; 

while_stmt
    : WHILE_IDENT '(' expr ')' stmt // 31
    ; 

WHILE_IDENT
    : WHILE // 32
    ;

block_stmt
    : '{' stmt_list '}' // 33
    ; 

compound_stmt
    : '{' local_decls stmt_list '}' // 34
    ; 

local_decls
    : local_decls local_decl // 35
    | // 36
    ; 

local_decl
    : type_spec IDENT ';' // 37
    | type_spec IDENT '[' int_literal ']' ';' // 38
    ;

if_stmt
    : IF '(' expr ')' stmt %prec UMINUS // 39
    // if_stmt : IF '(' expr ')'
    //      flag = assign_new_variable()
    //      [0].trueLbl = assign_new_label(2)
    //      [0].endLbl  = assign_new_label(2 + [4].code.size() + 1)
    //      code = {
    //          codegen("{} = neq {} {}", flag, [3].var, zero), 
    //          codegen("branch {} {} {}", [0].trueLbl, [0].endLbl), 
    //          [4].code, 
    //          codegen("goto {}", [0].endLbl)
    //      }
    | IF '(' expr ')' stmt ELSE stmt %prec MPR // 40
    ;

return_stmt
    : RETURN ';' // 41
    | RETURN expr ';' // 42
    ;

// [expr].var
expr
    : expr OR expr // 43
    | expr EQ expr// 44
    | expr NE expr // 45
    | expr LE expr// 46
    | expr '<' expr// 47
    | expr GE expr// 48
    | expr '>' expr // 49
    | expr AND expr  // 50
    | expr '+' expr// 51
    // expr : expr '+' expr
    //      [0].var <- assign_new_variable()
    //      [0].code <- {
    //          codegen("{} = add {} {}", [0].var, [1].var, [2].var)
    //      }
    | expr '-' expr // 52
    | expr '*' expr// 53
    | expr '/' expr// 54
    | expr '%' expr  // 55
    | '!' expr %prec UMINUS// 56
    | '-' expr %prec UMINUS// 57
    | '+' expr %prec UMINUS// 58
    | '$' expr %prec UMINUS // 59
    | '(' expr ')'// 60
    | IDENT // 61
    | IDENT '[' expr ']' // 62
    // expr : IDENT '[' expr ']'
    //      v_width <- assign_new_variable()
    //      v_offset <- assign_new_variable()
    //      v_addr <- assign_new_variable()
    //      [0].var <- assign_new_variable()
    //      [0].code <- {
    //          codegen("{} = assign {}", v_width, 4), 
    //          codegen("{} = mult {} {}", v_offset, [3].var, v_width), 
    //          codegen("{} = add {} {}", v_addr, [0].var, v_offset), 
    //          codegen("{} = load {} {}", [0].var, v_addr, zero), 
    //      }
    | IDENT '(' args_ ')' // 63
    // expr : IDENT '(' args_ ')' 
    //      assert([0].var in func_decl_table)
    //      assert([2].argc == func_decl_table.at([0].var).argc)
    //      [0].var <- assign_new_variable()
    //      codegen("{} = call {}", [0].var, IDENT.var)
    | int_literal // 64
    // expr : int_literal
    //      [0].var = assign_new_variable()
    //      codegen("{} = assign {}", [0].var, [1].cval)
    | expr '&' expr // 65
    | expr '^' expr // 66
    | '~' expr // 67
    | expr LSHIFT expr // 68
    | expr RSHIFT expr // 69
    | expr '|' expr  // 70
    ;

// [int_literal].cval
int_literal 
    : DECNUM // 71
    // int_literal : DECNUM
    //      [0].cval <- atoi([1].lval)
    | HEXNUM // 72
    ; 

// [arg_list].argc
arg_list
    : arg_list ',' expr // 73
    // arg_list : arg_list ',' expr
    //      [0].argc <- [1].argc + 1
    //      codegen("push {}", [3].var)
    | expr // 74
    ;

// [arg_list].argc
args_
    : arg_list // 75
    // args_ : arg_list
    //      [args_].argc <- [arg_list].argc
    | // 76
    ;

// use inherited attribute (FUCK)
continue_stmt
    : CONTINUE ';' // 77
    // continue_stmt : CONTINUE '; 
    //      [0].
    ;

break_stmt
    : BREAK ';' // 78
    // break_stmt : BREAK ';'
    //      [0].BREAK = 
    ;


%%