%token IDENT VOID_ INT_ WHILE IF ELSE RETURN EQ NE LE GE AND OR DECNUM CONTINUE BREAK HEXNUM LSHIFT RSHIFT FOR

%left OR
%left AND
%left EQ NE LE GE '<' '>'
%left '+' '-'
%left '|'
%left '&' '^'
%left '*' '/' '%' 
%right LSHIFT RSHIFT
%right '!'
%right '~'
%nonassoc UMINUS
%nonassoc MPR
%start program

%%

program
    : decl_list
    ;  /*程序由变量描述或函数描述组成（decl）*/

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
    | type_spec IDENT '=' int_literal ';'
    | type_spec IDENT '[' int_literal ']' '=' '{' init_list '}' ';'
    ;  /*变量包括简单变量和一维数组变量*/

init_list
    : init_list ',' int_literal
    | int_literal
    ; /* 定义时赋值 */

type_spec
    : VOID_
    | INT_
    ;  /*函数返回值类型或变量类型包括整型或 VOID_*/

fun_decl
    : type_spec FUNCTION_IDENT '(' params ')' compound_stmt
    | type_spec FUNCTION_IDENT '(' params ')' ';'

    ;  /*要考虑设置全局函数信息为假，和函数表为申明*/

FUNCTION_IDENT
    : IDENT
    ;  /*建立全局函数名变量*/

params
    : param_list
    | VOID_
    ;  /*函数参数个数可为 0 或多个*/

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
    | for_stmt
    | return_stmt
    | continue_stmt
    | break_stmt
    | ';'
    ;


expr_stmt
    : IDENT '=' expr ';'
    | IDENT '[' expr ']' '=' expr ';'
    | '$' expr '=' expr ';'
    | IDENT '(' args_ ')' ';'
    ;  /*赋值语句*/

while_stmt
    : WHILE_IDENT '(' expr ')' stmt
    ;  /*WHILE 语句*/

WHILE_IDENT
    : WHILE
    ; /*建立入口出口信息全局变量*/

for_stmt
    : FOR '(' stmt ';' stmt ';' stmt ')' stmt
    ;

block_stmt
    : '{' local_decls stmt_list '}'
    ;  /*语句块*/

compound_stmt
    : '{' local_decls stmt_list '}'
    ;  /*函数内部描述，包括局部变量和语句描述*/

local_decls
    : local_decls local_decl
    |
    ;  /*函数内部变量描述*/

local_decl
    : type_spec IDENT ';'
    | type_spec IDENT '[' int_literal ']' ';'
    | type_spec IDENT '=' int_literal ';'
    | type_spec IDENT '[' int_literal ']' '=' '{' init_list '}' ';'
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
    : expr OR expr  /*逻辑或表达式，运算符为'||'*/
    | expr EQ expr
    | expr NE expr  /*关系表达式*/
    | expr LE expr
    | expr '<' expr
    | expr GE expr
    | expr '>' expr  /*关系表达式*/
    | expr AND expr  /*逻辑与表达式，运算符为’&&’**/
    | expr '+' expr
    | expr '-' expr  /*算术表达式*/
    | expr '*' expr
    | expr '/' expr
    | expr '%' expr  /*算术表达式*/
    | '!' expr %prec UMINUS
    | '-' expr %prec UMINUS
    | '+' expr %prec UMINUS
    | '$' expr %prec UMINUS  /*$ expr 为取端口地址为 expr 值的端口值*/
    | '(' expr ')'
    | IDENT
    | IDENT '[' expr ']'
    | IDENT '(' args_ ')'  /* IDENT ( args_ )为函数调用*/
    | int_literal  /*数值常量*/
    | expr '&' expr  /*按位与*/
    | expr '^' expr  /*按位异或*/
    | '~' expr  /*按位取反*/
    | expr LSHIFT expr  /*逻辑左移*/
    | expr RSHIFT expr  /*逻辑右移*/
    | expr '|' expr  /*按位或*/
    ;

int_literal
    : DECNUM
    | HEXNUM
    ;  /*数值常量是十进制整数*/

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