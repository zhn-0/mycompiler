%{
#include "tac.h"
#include "mini.y.h"
%}

%option yylineno

%%

"int"  {  return INT;  }

"printf"  {  return PRINT;  }

"return"  {  return RETURN;  }

"continue"  {  return CONTINUE;  }

"if"  {  return IF;  }

"else"  {  return ELSE;  }

"while"  {  return WHILE;  }

"goto"  {  return GOTO;  }

"for"  {  return FOR;  }

[A-Za-z]([A-Za-z]|[0-9])*  {  
	yylval.string = strdup(yytext); 
	return IDENTIFIER;
}

[0-9]*	{
	yylval.string = strdup(yytext); 
	return INTEGER;
}

\"[^\"]*\"  {
	yylval.string = strdup(yytext); 
	return TEXT;
}

"=="  {  return EQ;  }

"!="  {  return NE;  }

"<="  {  return LE;  }

"<"  {  return LT;  }

">="  {  return GE;  }

">"  {  return GT;  }

"++"  {  return DA;  }

"--"  {  return DM;  }

[ \t\r\n]|#.*|\/\/.*

. 	{	return *yytext; }

%%

int yywrap()
{
	return 1;
}

