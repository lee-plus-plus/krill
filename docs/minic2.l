digit ([0-9])|([1-9][0-9]*)
letter [a-zA-Z]
id {letter}({letter}|{digit})*
num {digit}[1-9]*
hex 0(x|X)({letter}|[0-9])*

%%

"void" {return VOID;}
"continue" {return CONTINUE;}
"if" {return IF; }
"while" {return WHILE;}
"else" {return ELSE;}
"break" {return BREAK;}
"int" {return INT;}
"return" {return RETURN;}
"\|\|" {return OR;}
"&&" {return AND;}
{id} {yyval.IDENT_v.ID_NAME=yytext;return IDENT;}
{hex} {yyval.int_literal_v.int_val=ConvertHexToint(yytext);return HEXNUM;}
{num} {yyval.int_literal_v.int_val=atoi(yytext);return DECNUM;}
"\<=" {return LE;}
"\>=" {return GE;}
"\=\=" {return EQ;}
"\!\=" {return NE;}
"\>" {return '>';}
"\<" {return '<';}
"\," {return ','; }
"\;" {return ';'; }
"\{" {return '{'; }
"\}" {return '}'; }
"\%" {return '%'; }
"\*" {return '*';}
"\+" {return '+'; }
"\-" {return '-'; }
"/" {return '/'; }
"\=" {return '='; }
"\(" {return '('; }
"\)" {return ')'; }
"~" {return '~';}
"&" {return '&';}
"^" {return '^';}
"\[" {return '[';}
"\]" {return ']';}
"\<\<" {return LSHIFT;}
"\>\>" {return RSHIFT;}
"\|" {return '|';}
\t|\ { }
\n|\r\n {Lineno++;yylineno++;}
"$" {return '$';}


