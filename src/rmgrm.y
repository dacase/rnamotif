%{

#include <stdio.h>
#include "rnamot.h"

extern	VALUE_T	rm_tokval;

#define	CTX_START	0
#define	CTX_PARMS	1
#define	CTX_DESCR	2
#define	CTX_SITES	3
#define	CTX_SCORE	4

static	int	context = CTX_START;

static	NODE_T	*np;

%}

%token	SYM_PARMS
%token	SYM_DESCR
%token	SYM_SITES
%token	SYM_SCORE

%token	SYM_SS
%token	SYM_H5
%token	SYM_H3
%token	SYM_P5
%token	SYM_P3
%token	SYM_T1
%token	SYM_T2
%token	SYM_T3
%token	SYM_Q1
%token	SYM_Q2
%token	SYM_Q3
%token	SYM_Q4

%token	SYM_ACCEPT
%token	SYM_REJECT
%token	SYM_ELSE
%token	SYM_FOR
%token	SYM_IF
%token	SYM_IN
%token	SYM_WHILE

%token	SYM_IDENT
%token	SYM_INT
%token	SYM_FLOAT
%token	SYM_STRING

%token	SYM_AND
%token	SYM_ASSIGN
%token	SYM_DOLLAR
%token	SYM_DONT_MATCH
%token	SYM_EQUAL
%token	SYM_GREATER
%token	SYM_GREATER_EQUAL
%token	SYM_LESS
%token	SYM_LESS_EQUAL
%token	SYM_MATCH
%token	SYM_MINUS
%token	SYM_MINUS_ASSIGN
%token	SYM_MINUS_MINUS
%token	SYM_NOT
%token	SYM_NOT_EQUAL
%token	SYM_OR
%token	SYM_PERCENT
%token	SYM_PERCENT_ASSIGN
%token	SYM_PLUS
%token	SYM_PLUS_ASSIGN
%token	SYM_PLUS_PLUS
%token	SYM_STAR
%token	SYM_STAR_ASSIGN
%token	SYM_SLASH
%token	SYM_SLASH_ASSIGN

%token	SYM_LPAREN
%token	SYM_RPAREN
%token	SYM_LCURLY
%token	SYM_RCURLY
%token	SYM_COLON
%token	SYM_COMMA
%token	SYM_SEMICOLON

%token	SYM_CALL
%token	SYM_LIST
%token	SYM_STREF

%token	SYM_ERROR

%%
program		: parm_part descr_part site_part ;

parm_part	: SYM_PARMS { context = CTX_PARMS; } assign_list
		| ;
assign_list	: assign
		| assign_list assign;
assign		: ident assign_op expr
				{ $$ = updnode( $2, 0, $1, $3 );
				  if( context == CTX_PARMS )
					PARM_add( $$ );
				  else if( context == CTX_DESCR ||
					context == CTX_SITES )
					SE_addval( $$ ); } ;
assign_op	: SYM_ASSIGN	{ $$ = node( SYM_ASSIGN, 0, 0, 0 ); }
		| SYM_PLUS_ASSIGN
				{ $$ = node( SYM_PLUS_ASSIGN, 0, 0, 0 ); }
		| SYM_MINUS_ASSIGN
				{ $$ = node( SYM_MINUS_ASSIGN, 0, 0, 0 ); } ;
expr		: val 		{ $$ = $1; }
		| expr add_op val
				{ $$ = node( $2, 0, $1, $3 ); } ;
add_op		: SYM_PLUS	{ $$ = SYM_PLUS; }
		| SYM_MINUS 	{ $$ = SYM_MINUS; } ;

val		: ident		{ $$ = $1; }
		| pairval 	{ $$ = $1; }
		| SYM_INT	{ $$ = node( SYM_INT, &rm_tokval, 0, 0 ); }
		| SYM_FLOAT	{ $$ = node( SYM_FLOAT, &rm_tokval, 0, 0 ); }
		| SYM_DOLLAR	{ $$ = node( SYM_DOLLAR, &rm_tokval, 0, 0 ); }
		| SYM_STRING	{ $$ = node( SYM_STRING, &rm_tokval, 0, 0 ); } ;

ident		: SYM_IDENT 	{ $$ = node( SYM_IDENT, &rm_tokval, 0, 0 ); } ;

pairval		: SYM_LCURLY 	{ PR_open(); } pair_list SYM_RCURLY
				{ $$ = PR_close(); } ;
pair_list	: pair		{ PR_add( $1 ); }
		| pair_list SYM_COMMA pair
				{ PR_add( $3 ); } ;
pair		: SYM_STRING 	{ $$ = node( SYM_STRING, &rm_tokval, 0, 0 ); };

descr_part	: SYM_DESCR { context = CTX_DESCR; } strel_list ;
strel_list	: strel
		| strel strel_list ;
strel		: strhdr	{ if( context == CTX_DESCR )
					SE_close();
				  else if( context == CTX_SITES )
					POS_close( 0 ); }
		| strhdr SYM_LPAREN strparm_list SYM_RPAREN
				{ if( context == CTX_DESCR )
					SE_close();
				  else if( context == CTX_SITES )
					POS_close( 1 ); } ;
strhdr		: strtype	{ if( context == CTX_DESCR )
					SE_open( $1 );
				  else if( context == CTX_SITES )
					POS_open( $1 ); } ;
strtype		: SYM_SS	{ $$ = SYM_SS; }
		| SYM_H5	{ $$ = SYM_H5; }
		| SYM_H3	{ $$ = SYM_H3; }
		| SYM_P5	{ $$ = SYM_P5; }
		| SYM_P3	{ $$ = SYM_P3; }
		| SYM_T1	{ $$ = SYM_T1; }
		| SYM_T2	{ $$ = SYM_T2; }
		| SYM_T3	{ $$ = SYM_T3; }
		| SYM_Q1	{ $$ = SYM_Q1; }
		| SYM_Q2	{ $$ = SYM_Q2; }
		| SYM_Q3	{ $$ = SYM_Q3; }
		| SYM_Q4	{ $$ = SYM_Q4; } ;
strparm_list	: assign
		| assign SYM_COMMA strparm_list ;

site_part	: SYM_SITES { context = CTX_SITES; } site_list
		| ;
site_list	: site
		| site_list site ;
site		: siteaddr_list SYM_IN pairval
				{ SI_close( $3 ); } ;
siteaddr_list	: strel
		| siteaddr_list SYM_COLON strel ;
%%

#include "lex.yy.c"

int	yyerror( msg )
char	msg[];
{

	fprintf( stderr, "yyerror: %s\n", msg );
	return( 0 );
}
