%{

#include <stdio.h>
#include "rnamot.h"

extern	VALUE_T	rm_tokval;
extern	int	rm_context;

static	NODE_T	*np;

%}

%token	SYM_PARMS
%token	SYM_DESCR
%token	SYM_SITES
%token	SYM_SCORE

%token	SYM_SE
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
%token	SYM_BREAK
%token	SYM_CONTINUE
%token	SYM_ELSE
%token	SYM_FOR
%token	SYM_IF
%token	SYM_IN
%token	SYM_REJECT
%token	SYM_WHILE

%token	SYM_IDENT
%token	SYM_INT
%token	SYM_FLOAT
%token	SYM_STRING
%token	SYM_PAIRSET

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
%token	SYM_NEGATE
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
%token	SYM_LBRACK
%token	SYM_RBRACK
%token	SYM_LCURLY
%token	SYM_RCURLY
%token	SYM_COLON
%token	SYM_COMMA
%token	SYM_SEMICOLON

%token	SYM_CALL
%token	SYM_LIST
%token	SYM_KW_STREF
%token	SYM_IX_STREF

%token	SYM_ERROR

%%
program		: parm_part descr_part site_part score_part ;

parm_part	: SYM_PARMS { rm_context = CTX_PARMS; } pd_list
		| ;
descr_part	: SYM_DESCR { rm_context = CTX_DESCR; } se_list
		| ;
site_part	: SYM_SITES { rm_context = CTX_SITES; } kw_site_list
		| ;
score_part	: SYM_SCORE { rm_context = CTX_SCORE; } rule_list
				{ RM_accept(); }
		| ;

pd_list		: pdef
		| pdef pd_list ;
pdef		: asgn SYM_SEMICOLON ;

se_list		: strel
		| strel se_list ;
strel		: strhdr	{ if( rm_context == CTX_DESCR )
					SE_close();
				  else if( rm_context == CTX_SITES )
					POS_close();
				}
		| kw_stref ;
strhdr		: strtype	{ if( rm_context == CTX_DESCR )
					SE_open( $1 );
				  else if( rm_context == CTX_SITES )
					POS_open( $1 );
				  else
					$$ = RM_node( $1, 0, 0, 0 );
				} ;
strtype		: SYM_SE	{ $$ = SYM_SE; }
		| SYM_SS	{ $$ = SYM_SS; }
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

kw_site_list	: kw_site
		| kw_site_list kw_site ;
kw_site		: kw_pairing SYM_IN pairset 
				{ if( rm_context == CTX_SITES )
					SI_close( $3 );
				  else if( rm_context == CTX_SCORE )
					$$ = RM_node( SYM_IN, 0, $1, $3 );
				} ;
site		: pairing SYM_IN pairset 
				{ if( rm_context == CTX_SITES )
					SI_close( $3 );
				  else if( rm_context == CTX_SCORE )
					$$ = RM_node( SYM_IN, 0, $1, $3 );
				} ;

rule_list	: rule
		| rule rule_list ;
rule		: expr 		{ RM_action( $1 ); }
			action	{ RM_endaction(); } ;
		| action ;
action		: SYM_LCURLY stmt_list SYM_RCURLY ;
stmt_list	: stmt
		| stmt stmt_list ;
stmt		: accept_stmt
		| asgn_stmt
		| auto_stmt
		| break_stmt
		| call_stmt
		| cmpd_stmt
		| continue_stmt
		| empty_stmt
		| for_stmt
		| if_stmt
		| reject_stmt
		| while_stmt ;
accept_stmt	: SYM_ACCEPT SYM_SEMICOLON
				{ RM_accept(); } ;
asgn_stmt	: asgn SYM_SEMICOLON
				{ RM_mark();
				  RM_expr( 0, $1 );
				  RM_clear();
				} ;
auto_stmt	: auto_lval SYM_SEMICOLON
				{ RM_mark();
				  RM_expr( 0, $1 );
				  RM_clear();
				} ;
break_stmt	: SYM_BREAK SYM_SEMICOLON
				{ RM_break(); } ;
call_stmt	: fcall SYM_SEMICOLON
				{ RM_expr( 0, $1 );
				  RM_clear();
				} ;
cmpd_stmt	: SYM_LCURLY stmt_list SYM_RCURLY ;
continue_stmt	: SYM_CONTINUE SYM_SEMICOLON
				{ RM_continue(); } ;
empty_stmt	: empty SYM_SEMICOLON ;
for_stmt	: for_hdr stmt	{ RM_endfor(); } ;
if_stmt		: if_hdr stmt	{ RM_endif(); }
		| if_hdr stmt SYM_ELSE
				{ RM_else(); } stmt
				{ RM_endelse(); } ;
reject_stmt	: SYM_REJECT SYM_SEMICOLON
				{ RM_reject(); } ;
while_stmt	: SYM_WHILE SYM_LPAREN expr { RM_while( $3 ); }
			SYM_RPAREN stmt
				{ RM_endwhile(); } ;
if_hdr		: SYM_IF SYM_LPAREN expr { RM_if( $3 ); } SYM_RPAREN ;
for_hdr		: SYM_FOR SYM_LPAREN for_ctrl SYM_RPAREN ;
for_ctrl	: for_init	{ RM_forinit( $1 ); }
			 SYM_SEMICOLON for_test
				{ RM_fortest( $4 ); }
			SYM_SEMICOLON for_incr
				{ RM_forincr( $7 ); } ;
for_init	: asgn		{ $$ = $1; }
		| auto_lval	{ $$ = $1; }
		| empty 	{ $$ = $1; } ;
for_test	: asgn		{ $$ = $1; }
		| expr		{ $$ = $1; }
		| empty		{ $$ = $1; } ;
for_incr	: asgn		{ $$ = $1; }
		| auto_lval	{ $$ = $1; }
		| empty		{ $$ = $1; } ;

asgn		: lval asgn_op asgn
				{ $$ = RM_node( $2, 0, $1, $3 );
				  if( rm_context == CTX_PARMS )
					PARM_add( $$ );
				  else if( rm_context == CTX_DESCR ||
					rm_context == CTX_SITES )
					SE_addval( $$ );
				}
		| lval asgn_op expr
				{ $$ = RM_node( $2, 0, $1, $3 );
				  if( rm_context == CTX_PARMS )
					PARM_add( $$ );
				  else if( rm_context == CTX_DESCR ||
					rm_context == CTX_SITES )
					SE_addval( $$ );
				} ;
asgn_op		: SYM_ASSIGN	{ $$ = SYM_ASSIGN; }
		| SYM_MINUS_ASSIGN
				{ $$ = SYM_MINUS_ASSIGN; }
		| SYM_PLUS_ASSIGN
				{ $$ = SYM_PLUS_ASSIGN; }
		| SYM_PERCENT_ASSIGN
				{ $$ = SYM_PERCENT_ASSIGN; }
		| SYM_SLASH_ASSIGN
				{ $$ = SYM_SLASH_ASSIGN; }
		| SYM_STAR_ASSIGN
				{ $$ = SYM_STAR_ASSIGN; } ;
expr		: conj		{ $$ = $1; }
		| expr SYM_OR conj
				{ $$ = RM_node( SYM_OR, 0, $1, $3 ); } ;
conj		: compare	{ $$ = $1; }
		| compare SYM_AND conj
				{ $$ = RM_node( SYM_AND, 0, $1, $3 ); } ;
compare		: site		{ $$ = $1; }
		| a_expr	{ $$ = $1; }
		| a_expr comp_op a_expr
				{ $$ = RM_node( $2, 0, $1, $3 ); } ;
comp_op		: SYM_DONT_MATCH
				{ $$ = SYM_DONT_MATCH; }
		| SYM_EQUAL	{ $$ = SYM_EQUAL; }
		| SYM_GREATER	{ $$ = SYM_GREATER; }
		| SYM_GREATER_EQUAL
				{ $$ = SYM_GREATER_EQUAL; }
		| SYM_LESS	{ $$ = SYM_LESS; }
		| SYM_LESS_EQUAL
				{ $$ = SYM_LESS_EQUAL; }
		| SYM_MATCH	{ $$ = SYM_MATCH; }
		| SYM_NOT_EQUAL	{ $$ = SYM_NOT_EQUAL; } ;
a_expr		: term		{ $$ = $1; }
		| a_expr add_op term
				{ $$ = RM_node( $2, 0, $1, $3 ); } ;
add_op		: SYM_PLUS	{ $$ = SYM_PLUS; }
		| SYM_MINUS 	{ $$ = SYM_MINUS; } ;
term		: factor	{ $$ = $1; }
		| term mul_op factor
				{ $$ = RM_node( $2, 0, $1, $3 ); } ;
mul_op		: SYM_PERCENT	{ $$ = SYM_PERCENT; }
		| SYM_SLASH	{ $$ = SYM_SLASH; }
		| SYM_STAR	{ $$ = SYM_STAR; } ;
factor		: primary	{ $$ = $1; }
		| SYM_MINUS primary
				{ $$ = RM_node( SYM_NEGATE, 0, 0, $2 ); }
		| SYM_NOT primary
				{ $$ = RM_node( SYM_NOT, 0, 0, $2 ); }
		| stref		{ if( rm_context == CTX_SCORE )
					$$ = $1;
				} ;
pairing		: stref		{ if( rm_context == CTX_SCORE )
					$$ = $1;
				}
		| stref SYM_COLON pairing
				{ if( rm_context == CTX_SCORE )
					$$ = RM_node( SYM_COLON, 0, $1, $3 );
				} ;
kw_pairing 	: kw_stref	{ if( rm_context == CTX_SCORE )
					$$ = $1;
				}
		| kw_stref SYM_COLON kw_pairing
				{ if( rm_context == CTX_SCORE )
					$$ = RM_node( SYM_COLON, 0, $1, $3 );
				} ;
primary		: lval		{ $$ = $1; }
		| literal	{ $$ = $1; }
		| fcall		{ $$ = $1; }
		| SYM_LPAREN expr SYM_RPAREN
				{ $$ = $2; } ;
fcall		: ident SYM_LPAREN e_list SYM_RPAREN
				{ $$ = RM_node( SYM_CALL, 0, $1, $3 ); } ;
stref		: kw_stref	{ $$ = $1; }
		| ix_stref	{ $$ = $1; } ;
kw_stref	: strhdr SYM_LPAREN a_list SYM_RPAREN
				{ if( rm_context == CTX_DESCR )
					SE_close();
				  else if( rm_context == CTX_SITES )
					POS_close();
				  else if( rm_context == CTX_SCORE )
					$$ = RM_node( SYM_KW_STREF, 0, $1, $3 );
				} ;
ix_stref	: strhdr SYM_LBRACK e_list SYM_RBRACK
				{ $$ = RM_node( SYM_IX_STREF, 0, $1, $3 ); } ;
lval		: ident		{ $$ = $1; }
		| auto_lval	{ $$ = $1; } ;
auto_lval	: incr_op ident	{ $$ = RM_node( $1, 0, 0, $2 ); }
		| ident incr_op	{ $$ = RM_node( $2, 0, $1, 0 ); } ;
literal		: SYM_INT	{ $$ = RM_node( SYM_INT, &rm_tokval, 0, 0 ); }
		| SYM_FLOAT	{ $$ = RM_node( SYM_FLOAT, &rm_tokval, 0, 0 ); }
		| SYM_DOLLAR	{ $$ = RM_node( SYM_DOLLAR,
					&rm_tokval, 0, 0 ); }
		| string	{ $$ = $1; }
		| pairset	{ $$ = $1; } ;
ident		: SYM_IDENT 	{ $$ = RM_node( SYM_IDENT,
					&rm_tokval, 0, 0 ); } ;
incr_op		: SYM_MINUS_MINUS
				{ $$ = SYM_MINUS_MINUS; }
		| SYM_PLUS_PLUS	{ $$ = SYM_PLUS_PLUS; } ;
e_list		: expr		{ $$ = RM_node( SYM_LIST, 0, $1, 0 ); }
		| expr SYM_COMMA e_list
				{ $$ = RM_node( SYM_LIST, 0, $1, $3 ); } ;
a_list		: asgn		{ if( rm_context == CTX_SCORE )
					$$ = RM_node( SYM_LIST, 0, $1, 0 );
				}
		| asgn SYM_COMMA a_list
				{ if( rm_context == CTX_SCORE )
					$$ = RM_node( SYM_LIST, 0, $1, $3 );
				} ;
pairset		: SYM_LCURLY 	{ PR_open(); }
			s_list SYM_RCURLY
				{ $$ = PR_close(); } ;
s_list		: string	{ PR_add( $$ ); }
		| string SYM_COMMA s_list
				{ PR_add( $1 ) ; } ;
string		: SYM_STRING	{ $$ = RM_node( SYM_STRING,
					&rm_tokval, 0, 0 ); } 
empty		: 		{ $$ = (  int )NULL; } ;
%%

#include "lex.yy.c"

int	yyerror( msg )
char	msg[];
{

	fprintf( stderr, "yyerror: %s\n", msg );
	return( 0 );
}
