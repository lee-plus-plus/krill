"/*"            { comment(); }
"//"[^\n\r]*    { /* consume //-comment */ }


"auto"          { count(); return(AUTO); }
"_Bool"         { count(); return(BOOL); }
"break"         { count(); return(BREAK); }
"case"          { count(); return(CASE); }
"char"          { count(); return(CHAR); }
"_Complex"      { count(); return(COMPLEX); }
"const"         { count(); return(CONST); }
"continue"      { count(); return(CONTINUE); }
"default"       { count(); return(DEFAULT); }
"do"            { count(); return(DO); }
"double"        { count(); return(DOUBLE); }
"else"          { count(); return(ELSE); }
"enum"          { count(); return(ENUM); }
"extern"        { count(); return(EXTERN); }
"float"         { count(); return(FLOAT); }
"for"           { count(); return(FOR); }
"goto"          { count(); return(GOTO); }
"if"            { count(); return(IF); }
"_Imaginary"    { count(); return(IMAGINARY); }
"inline"        { count(); return(INLINE); }
"int"           { count(); return(INT); }
"long"          { count(); return(LONG); }
"register"      { count(); return(REGISTER); }
"restrict"      { count(); return(RESTRICT); }
"return"        { count(); return(RETURN); }
"short"         { count(); return(SHORT); }
"signed"        { count(); return(SIGNED); }
"sizeof"        { count(); return(SIZEOF); }
"static"        { count(); return(STATIC); }
"struct"        { count(); return(STRUCT); }
"switch"        { count(); return(SWITCH); }
"typedef"       { count(); return(TYPEDEF); }
"union"         { count(); return(UNION); }
"unsigned"      { count(); return(UNSIGNED); }
"void"          { count(); return(VOID); }
"volatile"      { count(); return(VOLATILE); }
"while"         { count(); return(WHILE); }

[a-zA-Z_]([a-zA-Z_]|[0-9])*                                     { count(); return(check_type()); }

0[xX][a-fA-F0-9]+((u|U)|(u|U)?(l|L|ll|LL)|(l|L|ll|LL)(u|U))?    { count(); return(CONSTANT); }
0[0-7]*((u|U)|(u|U)?(l|L|ll|LL)|(l|L|ll|LL)(u|U))?              { count(); return(CONSTANT); }
[1-9][0-9]*((u|U)|(u|U)?(l|L|ll|LL)|(l|L|ll|LL)(u|U))?          { count(); return(CONSTANT); }
L?'(\\.|[^\\'\n\r])+'                                           { count(); return(CONSTANT); }

[0-9]+([Ee][+-]?[0-9]+)(f|F|l|L)?                               { count(); return(CONSTANT); }
[0-9]*"."[0-9]+([Ee][+-]?[0-9]+)?(f|F|l|L)?                     { count(); return(CONSTANT); }
[0-9]+"."[0-9]*([Ee][+-]?[0-9]+)?(f|F|l|L)?                     { count(); return(CONSTANT); }
0[xX][a-fA-F0-9]+([Pp][+-]?[0-9]+)(f|F|l|L)?                    { count(); return(CONSTANT); }
0[xX][a-fA-F0-9]*"."[a-fA-F0-9]+([Pp][+-]?[0-9]+)?(f|F|l|L)?    { count(); return(CONSTANT); }
0[xX][a-fA-F0-9]+"."[a-fA-F0-9]*([Pp][+-]?[0-9]+)?(f|F|l|L)?    { count(); return(CONSTANT); }


L?\"(\\.|[^\\\"\n\r])*\"    { count(); return(STRING_LITERAL); }

"..."           { count(); return(ELLIPSIS); }
">>="           { count(); return(RIGHT_ASSIGN); }
"<<="           { count(); return(LEFT_ASSIGN); }
"+="            { count(); return(ADD_ASSIGN); }
"-="            { count(); return(SUB_ASSIGN); }
"*="            { count(); return(MUL_ASSIGN); }
"/="            { count(); return(DIV_ASSIGN); }
"%="            { count(); return(MOD_ASSIGN); }
"&="            { count(); return(AND_ASSIGN); }
"^="            { count(); return(XOR_ASSIGN); }
"|="            { count(); return(OR_ASSIGN); }
">>"            { count(); return(RIGHT_OP); }
"<<"            { count(); return(LEFT_OP); }
"++"            { count(); return(INC_OP); }
"--"            { count(); return(DEC_OP); }
"->"            { count(); return(PTR_OP); }
"&&"            { count(); return(AND_OP); }
"||"            { count(); return(OR_OP); }
"<="            { count(); return(LE_OP); }
">="            { count(); return(GE_OP); }
"=="            { count(); return(EQ_OP); }
"!="            { count(); return(NE_OP); }

";"             { count(); return(';'); }
("{"|"<%")      { count(); return('{'); }
("}"|"%>")      { count(); return('}'); }
","             { count(); return(','); }
":"             { count(); return(':'); }
"="             { count(); return('='); }
"("             { count(); return('('); }
")"             { count(); return(')'); }
("["|"<:")      { count(); return('['); }
("]"|":>")      { count(); return(']'); }

"."         { count(); return('.'); }
"&"         { count(); return('&'); }
"!"         { count(); return('!'); }
"~"         { count(); return('~'); }
"-"         { count(); return('-'); }
"+"         { count(); return('+'); }
"*"         { count(); return('*'); }
"/"         { count(); return('/'); }
"%"         { count(); return('%'); }
"<"         { count(); return('<'); }
">"         { count(); return('>'); }
"^"         { count(); return('^'); }
"|"         { count(); return('|'); }
"?"         { count(); return('?'); }

[ \t\v\n\r\f]       { count(); }
.           { /* Add code to complain about unmatched characters */ }
