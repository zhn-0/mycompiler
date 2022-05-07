%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tac.h"
#include "obj.h"

int yylex();
void yyerror(char* msg);

%}

%union
{
	char character;
	char *string;
	SYM *sym;
	TAC *tac;
	EXP	*exp;
}

%token INT EQ NE LT LE GT GE UMINUS IF ELSE WHILE CONTINUE PRINT RETURN DM DA GOTO FOR
%token <string> INTEGER IDENTIFIER TEXT

%left EQ NE LT LE GT GE
%left '+' '-'
%left '*' '/'
%right UMINUS

%type <tac> program function_declaration_list function_declaration function parameter_list variable_list variable statement
%type <tac> assignment_statement print_statement print_list print_item return_statement null_statement if_statement while_statement
%type <tac> call_statement block declaration_list declaration statement_list
%type <tac> self_statement goto_statement label_statement for_statement for_declaration
%type <exp> argument_list expression_list expression call_expression
%type <sym> function_head

%%

program : function_declaration_list
{
	tac_last=$1;
	tac_complete();
}
;

function_declaration_list : function_declaration
| function_declaration_list function_declaration
{
	$$=join_tac($1, $2);
}
;

function_declaration : function
| declaration
;

declaration : INT variable_list ';'
{
	$$=$2;
}
;

variable_list : variable
{
	$$=$1;
}               
| variable_list ',' variable
{
	$$=join_tac($1, $3);
}               
;

variable : IDENTIFIER
{
	$$=declare_var($1);
}
| IDENTIFIER '=' expression
{
	$$=declare_var($1);
	$$=join_tac($$, do_assign(get_var($1), $3));
}
| IDENTIFIER '[' INTEGER ']'
{
	$$=declare_array($1, atoi($3));
}
;

function : function_head '(' parameter_list ')' block
{
	$$=do_func($1, $3, $5);
	scope_local=0; /* Leave local scope. */
	sym_tab_local=NULL; /* Clear local symbol table. */
} 
| error
{
	error("Bad function syntax");
}
;

function_head : IDENTIFIER
{
	$$=declare_func($1);
	scope_local=1; /* Enter local scope. */
	sym_tab_local=NULL; /* Init local symbol table. */
}
;

parameter_list : IDENTIFIER
{
	$$=declare_para($1);
}               
| parameter_list ',' IDENTIFIER
{
	$$=join_tac($1, declare_para($3));
}               
|
{
	$$=NULL;
}
;

statement : assignment_statement ';'
| call_statement ';'
| return_statement ';'
| print_statement ';'
| null_statement ';'
| self_statement ';'
| goto_statement ';'
| label_statement
| for_statement
| if_statement
| while_statement
| block
| error
{
	error("Bad statement syntax");
}
;

block : '{' declaration_list statement_list '}'
{
	$$=join_tac($2, $3);
}               
;

declaration_list:
{
	$$=NULL;
}
| declaration_list declaration
{
	$$=join_tac($1, $2);
}
;

statement_list : statement
| statement_list statement
{
	$$=join_tac($1, $2);
}               
;

self_statement :
  IDENTIFIER DA
{
	SYM* i=get_var($1);
	$$=mk_tac(TAC_ADD, i, i, mk_const(1));
}
| IDENTIFIER DM
{
	SYM* i=get_var($1);
	$$=mk_tac(TAC_SUB, i, i, mk_const(1));
}
| DA IDENTIFIER
{
	SYM* i=get_var($2);
	$$=mk_tac(TAC_ADD, i, i, mk_const(1));
}
| DM IDENTIFIER
{
	SYM* i=get_var($2);
	$$=mk_tac(TAC_SUB, i, i, mk_const(1));
}

assignment_statement : IDENTIFIER '=' expression
{
	$$=do_assign(get_var($1), $3);
}
| IDENTIFIER '[' INTEGER ']' '=' expression
{
	$$=do_assign(get_array_element($1, $3), $6);
}
;

expression : expression '+' expression
{
	$$=do_bin(TAC_ADD, $1, $3);
}
| expression '-' expression
{
	$$=do_bin(TAC_SUB, $1, $3);
}
| expression '*' expression
{
	$$=do_bin(TAC_MUL, $1, $3);
}
| expression '/' expression
{
	$$=do_bin(TAC_DIV, $1, $3);
}
| '-' expression  %prec UMINUS
{
	$$=do_un(TAC_NEG, $2);
}
| expression EQ expression
{
	$$=do_cmp(TAC_EQ, $1, $3);
}
| expression NE expression
{
	$$=do_cmp(TAC_NE, $1, $3);
}
| expression LT expression
{
	$$=do_cmp(TAC_LT, $1, $3);
}
| expression LE expression
{
	$$=do_cmp(TAC_LE, $1, $3);
}
| expression GT expression
{
	$$=do_cmp(TAC_GT, $1, $3);
}
| expression GE expression
{
	$$=do_cmp(TAC_GE, $1, $3);
}
| '(' expression ')'
{
	$$=$2;
}
| IDENTIFIER DA
{
	$$=do_self_op(TAC_ADD, get_var($1));
}
| IDENTIFIER DM
{
	$$=do_self_op(TAC_SUB, get_var($1));
}
| DA IDENTIFIER
{
	$$=mk_exp(NULL, get_var($2), mk_tac(TAC_ADD, get_var($2), get_var($2), mk_const(1)));
}
| DM IDENTIFIER
{
	$$=mk_exp(NULL, get_var($2), mk_tac(TAC_SUB, get_var($2), get_var($2), mk_const(1)));
}               
| INTEGER
{
	$$=mk_exp(NULL, mk_const(atoi($1)), NULL);
}
| IDENTIFIER
{
	$$=mk_exp(NULL, get_var($1), NULL);
}
| IDENTIFIER '[' INTEGER ']'
{
	$$=mk_exp(NULL, get_array_element($1, $3), NULL);
}
| call_expression
{
	$$=$1;
}               
| error
{
	error("Bad expression syntax");
	$$=mk_exp(NULL, NULL, NULL);
}
;

argument_list           :
{
	$$=NULL;
}
| expression_list
;

expression_list : expression
|  expression_list ',' expression
{
	$3->next=$1;
	$$=$3;
}
;

print_statement : PRINT '(' print_list ')'
{
	$$=$3;
}               
;

print_list : print_item
| print_list ',' print_item
{
	$$=join_tac($1, $3);
}               
;

print_item : expression
{
	$$=join_tac($1->tac,
	do_lib("PRINTN", $1->ret));
}
| TEXT
{
	$$=do_lib("PRINTS", mk_text($1));
}
;

return_statement : RETURN expression
{
	TAC *t=mk_tac(TAC_RETURN, $2->ret, NULL, NULL);
	t->prev=$2->tac;
	$$=t;
}               
;

null_statement : CONTINUE
{
	$$=NULL;
}               
;

if_statement : IF '(' expression ')' block
{
	$$=do_if($3, $5);
}
| IF '(' expression ')' block ELSE block
{
	$$=do_test($3, $5, $7);
}
;

while_statement : WHILE '(' expression ')' block
{
	$$=do_while($3, $5);
}               
;

goto_statement : GOTO IDENTIFIER
{
	$$=mk_tac(TAC_GOTO, get_label($2), NULL, NULL);
}
;

label_statement : IDENTIFIER ':'
{
	$$=mk_tac(TAC_LABEL, mk_goto_label($1, 1), NULL, NULL);
}
;

for_declaration : 
{
	$$=NULL;
}
| INT variable_list
{
	$$=$2;
}
;

for_statement : FOR '(' for_declaration ';' expression ';' expression ')' block
{
	$$=do_for($3, $5, $7, $9);
}
;

call_statement : IDENTIFIER '(' argument_list ')'
{
	$$=do_call($1, $3);
}
;

call_expression : IDENTIFIER '(' argument_list ')'
{
	$$=do_call_ret($1, $3);
}
;

%%

void yyerror(char* msg) 
{
	fprintf(stderr, "%s: line %d\n", msg, yylineno);
	exit(0);
}

int main(int argc,   char *argv[])
{
	if(argc != 2)
	{
		printf("usage: %s filename\n", argv[0]);
		exit(0);
	}
	
	char *input, *output;

	input = argv[1];
	if(freopen(input, "r", stdin)==NULL)
	{
		printf("error: open %s failed\n", input);
		return 0;
	}

	output=(char *)malloc(strlen(input + 10));
	strcpy(output,input);
	strcat(output,".s");

	if(freopen(output, "w", stdout)==NULL)
	{
		printf("error: open %s failed\n", output);
		return 0;
	}

	tac_init();
	init_lru();

	yyparse();
	
	SYM *sym;
	if((sym=check_goto_label())!=NULL){
		fprintf(stderr, "error: undeclared label: %s\n", sym->name);
		return 0;
	}

	tac_obj();

	return 0;
}
