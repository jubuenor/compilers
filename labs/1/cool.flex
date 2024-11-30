/*
 *  The scanner definition for COOL.
 */

/*
 *  Stuff enclosed in %{ %} in the first section is copied verbatim to the
 *  output, so headers and global definitions are placed here to be visible
 * to the code in the file.  Don't remove anything that was here initially
 */

/*
 * DECLARATIONS
 */

%{
#include <cool-parse.h>
#include <stringtab.h>
#include <utilities.h>

/* The compiler assumes these identifiers. */
#define yylval cool_yylval
#define yylex  cool_yylex

/* Max size of string constants */
#define MAX_STR_CONST 1025
#define YY_NO_UNPUT   /* keep g++ happy */

extern FILE *fin; /* we read from this file */

/* define YY_INPUT so we read from the FILE fin:
 * This change makes it possible to use this scanner in
 * the Cool compiler.
 */
#undef YY_INPUT
#define YY_INPUT(buf,result,max_size) \
        if ( (result = fread( (char*)buf, sizeof(char), max_size, fin)) < 0) \
                YY_FATAL_ERROR( "read() in flex scanner failed");

char string_buf[MAX_STR_CONST]; /* to assemble string constants */
char *string_buf_ptr;

extern int curr_lineno;
extern int verbose_flag;

/*
 * how many open comments are.
 */
int comment_count = 0;

std::string complete_str = "";


extern YYSTYPE cool_yylval;

/*
 *  Add Your own definitions here
 */

%}

%option noyywrap
%x COMMENT STRING


/*
 * DEFINITIONS
 * Define names for regular expressions here.
 */

DARROW          =>
ASSIGN          <-
LE              <=

DIGIT           [0-9]
DIGITS			{DIGIT}+
LETTER          [a-zA-Z_]
TRUE            t[Rr][Uu][Ee]
FALSE           f[Aa][Ll][Ss][Ee]
/* TRUE            true
   FALSE           false*/
/* NEWLINE         (\n|\r\n)+*/
WHITESPACE      [ \t\b\v\r]
DASHCOMMENT     --.*

/*
  *  String constants (C syntax)
  *  Escape sequence \c is accepted for all characters c. Except for 
  *  \n \t \b \f, the result is c.
  *
  */

STRINGTEXT		(\\{WHITESPACE}+|\\\"|[^\"\n])*

 /*
  * Keywords are case-insensitive except for the values true and false,
  * which must begin with a lower-case letter.
  */

CLASS           (?i:class)
ELSE            (?i:else)
FI              (?i:fi)
IF              (?i:if)
IN              (?i:in)
INHERITS        (?i:inherits)
ISVOID          (?i:isvoid)
LET             (?i:let)
LOOP            (?i:loop)
POOL            (?i:pool)
THEN            (?i:then)
WHILE           (?i:while)
CASE            (?i:case)
ESAC            (?i:esac)
NEW             (?i:new)
OF              (?i:of)
NOT             (?i:not)

TYPEID          [A-Z]({DIGIT}|{LETTER})*
OBJECTID        [a-z]({DIGIT}|{LETTER})*


%%


 /*
  *  RULES
  *
  *  The multiple-character operators.
  */

{DARROW} { return DARROW; }
{LE} { return LE; }
{ASSIGN} { return ASSIGN; }

 /*
  *  Single-character operators.
  */

[{}();:,.+\-*\/~<=@] { return yytext[0]; }

 /*
  *  Keywords.
  */

{CLASS} { return CLASS; }
{ELSE} { return ELSE; }
{FI} { return FI; }
{IF} { return IF; }
{IN} { return IN; }
{INHERITS} { return INHERITS; }
{ISVOID} { return ISVOID; }
{LET} { return LET; }
{LOOP} { return LOOP; }
{POOL} { return POOL; }
{THEN} { return THEN; }
{WHILE} { return WHILE; }
{CASE} { return CASE; }
{ESAC} { return ESAC; }
{NEW} { return NEW; }
{OF} { return OF; }
{NOT} { return NOT; }

 /*
  * Whitespaces and newline.
  */

{WHITESPACE}+ { /* ignore */ }

\n { curr_lineno++; }

 /*
  * Constants.
  */  
 
{TRUE} { 
    cool_yylval.boolean = true;
    return BOOL_CONST;
}

{FALSE} {
    cool_yylval.boolean = false;
    return BOOL_CONST;
}
 
{DIGITS} { 
    cool_yylval.symbol = inttable.add_string(yytext);
    return INT_CONST;
}


\" {
	BEGIN(STRING);
}

<STRING>{STRINGTEXT} {
	cool_yylval.symbol = stringtable.add_string(yytext);
	return STR_CONST;
}

<STRING>\" {
	BEGIN(INITIAL);
}




 /*
  * Identifiers.
  */

{TYPEID} {
    cool_yylval.symbol = idtable.add_string(yytext);
    return TYPEID;
}

{OBJECTID} {
    cool_yylval.symbol = idtable.add_string(yytext);
    return OBJECTID;
}
 
 /*
  *  Comments.
  */

"(*" {
	comment_count++;
    BEGIN(COMMENT);
}

"*)" {
	cool_yylval.error_msg = "Unmatched *)";
	return ERROR;
}

<COMMENT>"(*" {
	comment_count++;
}

<COMMENT>"*)" {
	comment_count--;

	if (comment_count == 0) {
		BEGIN(INITIAL);
	}
}

<COMMENT>. { /* ignore */ }
 
<COMMENT><<EOF>> {
    BEGIN(INITIAL);
    cool_yylval.error_msg = "EOF in comment";
    return ERROR;
}

{DASHCOMMENT} {   }

 /*
  * Error class.
  */

. {
    cool_yylval.error_msg = yytext;
    return ERROR;
}


%%

/*
 * User subroutines
 */
