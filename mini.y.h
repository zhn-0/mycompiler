/* A Bison parser, made by GNU Bison 3.5.1.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2020 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* Undocumented macros, especially those whose name start with YY_,
   are private implementation details.  Do not rely on them.  */

#ifndef YY_YY_MINI_Y_H_INCLUDED
# define YY_YY_MINI_Y_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    INT = 258,
    EQ = 259,
    NE = 260,
    LT = 261,
    LE = 262,
    GT = 263,
    GE = 264,
    UMINUS = 265,
    IF = 266,
    ELSE = 267,
    WHILE = 268,
    CONTINUE = 269,
    PRINT = 270,
    RETURN = 271,
    DM = 272,
    DA = 273,
    GOTO = 274,
    FOR = 275,
    INTEGER = 276,
    IDENTIFIER = 277,
    TEXT = 278
  };
#endif
/* Tokens.  */
#define INT 258
#define EQ 259
#define NE 260
#define LT 261
#define LE 262
#define GT 263
#define GE 264
#define UMINUS 265
#define IF 266
#define ELSE 267
#define WHILE 268
#define CONTINUE 269
#define PRINT 270
#define RETURN 271
#define DM 272
#define DA 273
#define GOTO 274
#define FOR 275
#define INTEGER 276
#define IDENTIFIER 277
#define TEXT 278

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 14 "mini.y"

	char character;
	char *string;
	SYM *sym;
	TAC *tac;
	EXP	*exp;

#line 111 "mini.y.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);

#endif /* !YY_YY_MINI_Y_H_INCLUDED  */
