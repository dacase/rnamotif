%{

#include <stdio.h>
#include "rnamot.h"

extern	VALUE_T	rm_tokval;
extern	int	rm_context;

extern	void	RM_hold(NODE_T *);
extern	void	RM_release(NODE_T *);

/*
typedef	union	{
	int	ival;
	NODE_T	*npval;
} YYSTYPE;
*/

extern	int	yylex(void);
extern	int	yyerror(char *);

%}

%union	{
	int	ival;
	NODE_T	*npval;
}

%token	<ival>	SYM_PARMS
%token	<ival>	SYM_DESCR
%token	<ival>	SYM_SITES
%token	<ival>	SYM_SCORE

%token	<ival>	SYM_SE
%token	<ival>	SYM_CTX
%token	<ival>	SYM_SS
%token	<ival>	SYM_H5
%token	<ival>	SYM_H3
%token	<ival>	SYM_P5
%token	<ival>	SYM_P3
%token	<ival>	SYM_T1
%token	<ival>	SYM_T2
%token	<ival>	SYM_T3
%token	<ival>	SYM_Q1
%token	<ival>	SYM_Q2
%token	<ival>	SYM_Q3
%token	<ival>	SYM_Q4

%token	<ival>	SYM_ACCEPT
%token	<ival>	SYM_BEGIN
%token	<ival>	SYM_BREAK
%token	<ival>	SYM_CONTINUE
%token	<ival>	SYM_ELSE
%token	<ival>	SYM_END
%token	<ival>	SYM_FOR
%token	<ival>	SYM_HOLD
%token	<ival>	SYM_IF
%token	<ival>	SYM_IN
%token	<ival>	SYM_REJECT
%token	<ival>	SYM_RELEASE
%token	<ival>	SYM_WHILE

%token	<ival>	SYM_IDENT
%token	<ival>	SYM_INT
%token	<ival>	SYM_FLOAT
%token	<ival>	SYM_STRING
%token	<ival>	SYM_PAIRSET

%token	<ival>	SYM_AND
%token	<ival>	SYM_ASSIGN
%token	<ival>	SYM_DOLLAR
%token	<ival>	SYM_DONT_MATCH
%token	<ival>	SYM_EQUAL
%token	<ival>	SYM_GREATER
%token	<ival>	SYM_GREATER_EQUAL
%token	<ival>	SYM_LESS
%token	<ival>	SYM_LESS_EQUAL
%token	<ival>	SYM_MATCH
%token	<ival>	SYM_MINUS
%token	<ival>	SYM_MINUS_ASSIGN
%token	<ival>	SYM_MINUS_MINUS
%token	<ival>	SYM_NEGATE
%token	<ival>	SYM_NOT
%token	<ival>	SYM_NOT_EQUAL
%token	<ival>	SYM_OR
%token	<ival>	SYM_PERCENT
%token	<ival>	SYM_PERCENT_ASSIGN
%token	<ival>	SYM_PLUS
%token	<ival>	SYM_PLUS_ASSIGN
%token	<ival>	SYM_PLUS_PLUS
%token	<ival>	SYM_STAR
%token	<ival>	SYM_STAR_ASSIGN
%token	<ival>	SYM_SLASH
%token	<ival>	SYM_SLASH_ASSIGN

%token	<ival>	SYM_LPAREN
%token	<ival>	SYM_RPAREN
%token	<ival>	SYM_LBRACK
%token	<ival>	SYM_RBRACK
%token	<ival>	SYM_LCURLY
%token	<ival>	SYM_RCURLY
%token	<ival>	SYM_COLON
%token	<ival>	SYM_COMMA
%token	<ival>	SYM_SEMICOLON

%token	<ival>	SYM_CALL
%token	<ival>	SYM_LIST
%token	<ival>	SYM_KW_STREF
%token	<ival>	SYM_IX_STREF

%token	<ival>	SYM_ERROR

%type	<npval>	program
%type	<npval>	parm_part
%type	<npval>	parm_hdr
%type	<npval>	descr_part
%type	<npval>	site_part
%type	<npval>	score_part
%type	<npval>	pd_list
%type	<npval>	pdef
%type	<npval>	se_list
%type	<npval>	strel
%type	<npval>	strhdr
%type	<npval>	strtype
%type	<npval>	kw_site_list
%type	<npval>	kw_site
%type	<npval>	site
%type	<npval>	rule_list
%type	<npval>	rule
%type	<npval>	pattern
%type	<npval>	action
%type	<npval>	stmt_list
%type	<npval>	stmt
%type	<npval>	accept_stmt
%type	<npval>	asgn_stmt
%type	<npval>	auto_stmt
%type	<npval>	break_stmt
%type	<npval>	call_stmt
%type	<npval>	cmpd_stmt
%type	<npval>	continue_stmt
%type	<npval>	empty_stmt
%type	<npval>	for_stmt
%type	<npval>	if_stmt
%type	<npval>	hold_stmt
%type	<npval>	reject_stmt
%type	<npval>	release_stmt
%type	<npval>	while_stmt
%type	<npval>	loop_level
%type	<npval>	if_hdr
%type	<npval>	for_hdr
%type	<npval>	for_ctrl
%type	<npval>	for_init
%type	<npval>	for_test
%type	<npval>	for_incr
%type	<npval>	asgn
%type	<npval>	asgn_op
%type	<npval>	expr
%type	<npval>	conj
%type	<npval>	compare
%type	<npval>	comp_op
%type	<npval>	a_expr
%type	<npval>	add_op
%type	<npval>	term
%type	<npval>	mul_op
%type	<npval>	factor
%type	<npval>	pairing
%type	<npval>	kw_pairing
%type	<npval>	primary
%type	<npval>	fcall
%type	<npval>	stref
%type	<npval>	kw_stref
%type	<npval>	ix_stref
%type	<npval>	lval
%type	<npval>	auto_lval
%type	<npval>	literal
%type	<npval>	ident
%type	<npval>	incr_op
%type	<npval>	e_list
%type	<npval>	a_list
%type	<npval>	pairset
%type	<npval>	s_list
%type	<npval>	string
%type	<npval>	empty

%%
program		: parm_part descr_part site_part score_part 
		;

parm_part	: parm_hdr	{ rm_context = CTX_PARMS; }
			pd_list 
		| empty
		;
parm_hdr	: SYM_PARMS	{ $$ = NULL; }
		| empty		{ $$ = NULL; }
		;
descr_part	: SYM_DESCR	{ rm_context = CTX_DESCR; }
			  se_list
				{ $$ = NULL; }
		;
site_part	: SYM_SITES	{ rm_context = CTX_SITES; }
			kw_site_list
				{ $$ = NULL; }
		| { $$ = NULL; }
		;
score_part	: SYM_SCORE	{ rm_context = CTX_SCORE; }
			rule_list
				{ RM_accept(); $$ = NULL; }
		| { $$ = NULL; }
		;

pd_list		: pdef
		| pdef pd_list
		;
pdef		: asgn SYM_SEMICOLON
		;

se_list		: strel
		| strel se_list ;
strel		: strhdr	{ if( rm_context == CTX_DESCR )
					SE_close();
				  else if( rm_context == CTX_SITES )
					POS_close();
				}
		| kw_stref
		;
strhdr		: strtype	{ if( rm_context == CTX_DESCR )
					SE_open( $<ival>1 );
				  else if( rm_context == CTX_SITES )
					POS_open( $<ival>1 );
				  else
					$$ = RM_node( $<ival>1, 0, 0, 0 );
				}
		;
strtype		: SYM_SE	{ $<ival>$ = SYM_SE; }
		| SYM_CTX	{ $<ival>$ = SYM_CTX; }
		| SYM_SS	{ $<ival>$ = SYM_SS; }
		| SYM_H5	{ $<ival>$ = SYM_H5; }
		| SYM_H3	{ $<ival>$ = SYM_H3; }
		| SYM_P5	{ $<ival>$ = SYM_P5; }
		| SYM_P3	{ $<ival>$ = SYM_P3; }
		| SYM_T1	{ $<ival>$ = SYM_T1; }
		| SYM_T2	{ $<ival>$ = SYM_T2; }
		| SYM_T3	{ $<ival>$ = SYM_T3; }
		| SYM_Q1	{ $<ival>$ = SYM_Q1; }
		| SYM_Q2	{ $<ival>$ = SYM_Q2; }
		| SYM_Q3	{ $<ival>$ = SYM_Q3; }
		| SYM_Q4	{ $<ival>$ = SYM_Q4; }
		;

kw_site_list	: kw_site
		| kw_site_list kw_site
		;
kw_site		: kw_pairing SYM_IN pairset 
				{ if( rm_context == CTX_SITES )
					SI_close( $3 );
				  else if( rm_context == CTX_SCORE )
					$$ = RM_node( SYM_IN, 0, $1, $3 );
				}
		;
site		: pairing SYM_IN pairset 
				{ if( rm_context == CTX_SITES )
					SI_close( $3 );
				  else if( rm_context == CTX_SCORE )
					$$ = RM_node( SYM_IN, 0, $1, $3 );
				}
		;

rule_list	: rule
		| rule rule_list
		;
rule		: pattern	{ RM_action( $1 ); }
			action	{ RM_endaction(); }
		| action
		;
pattern		: SYM_BEGIN	{ $$ = RM_node( SYM_BEGIN, 0, 0, 0 ); }
		| SYM_END	{ $$ = RM_node( SYM_END, 0, 0, 0 ); }
		| expr		{ $$ = $1; }
		;
action		: SYM_LCURLY stmt_list SYM_RCURLY
				{ $$ = NULL; }
		;
stmt_list	: stmt
		| stmt stmt_list
		;
stmt		: accept_stmt
		| asgn_stmt
		| auto_stmt
		| break_stmt
		| call_stmt
		| cmpd_stmt
		| continue_stmt
		| empty_stmt
		| for_stmt
		| hold_stmt
		| if_stmt
		| reject_stmt
		| release_stmt
		| while_stmt
		;
accept_stmt	: SYM_ACCEPT SYM_SEMICOLON
				{ RM_accept(); $$ = NULL; }
		;
asgn_stmt	: asgn SYM_SEMICOLON
				{ RM_mark();
				  RM_expr( 0, $1 );
				  RM_clear();
				}
		;
auto_stmt	: auto_lval SYM_SEMICOLON
				{ RM_mark();
				  RM_expr( 0, $1 );
				  RM_clear();
				}
		;
break_stmt	: SYM_BREAK loop_level SYM_SEMICOLON
				{ RM_break( $2 ); $$ = NULL; }
		;
call_stmt	: fcall SYM_SEMICOLON
				{ RM_expr( 0, $1 );
				  RM_clear();
				}
		;
cmpd_stmt	: SYM_LCURLY stmt_list SYM_RCURLY
				{ $$ = NULL; }
		;
continue_stmt	: SYM_CONTINUE loop_level SYM_SEMICOLON
				{ RM_continue( $2 ); $$ = NULL; }
		;
empty_stmt	: empty SYM_SEMICOLON
		;
for_stmt	: for_hdr stmt	{ RM_endfor(); }
		;
hold_stmt	: SYM_HOLD ident SYM_SEMICOLON
			{ RM_hold( $2 ); }
		;
if_stmt		: if_hdr stmt	{ RM_endif(); }
		| if_hdr stmt SYM_ELSE
				{ RM_else(); }
			stmt
				{ RM_endelse(); }
		;
reject_stmt	: SYM_REJECT SYM_SEMICOLON
				{ RM_reject(); $$ = NULL; }
		;
release_stmt	: SYM_RELEASE ident SYM_SEMICOLON
			{ RM_release( $2 ); }
		;
while_stmt	: SYM_WHILE SYM_LPAREN expr
				{ RM_while( $3 ); $<npval>$ = NULL; }
			SYM_RPAREN stmt
				{ RM_endwhile(); $$ = NULL; }
		;
loop_level	: SYM_INT	{ $$ = RM_node( SYM_INT, &rm_tokval, 0, 0 ); }
		| 		{ $$ = NULL; }
		;
if_hdr		: SYM_IF SYM_LPAREN expr
				{ RM_if( $3 ); }
			SYM_RPAREN
				{ $$ = NULL; }
		;
for_hdr		: SYM_FOR SYM_LPAREN for_ctrl SYM_RPAREN
				{ $$ = NULL; }
		;
for_ctrl	: for_init	{  RM_forinit( $1 ); }
			SYM_SEMICOLON for_test
				{ RM_fortest( $4 ); }
			SYM_SEMICOLON for_incr
				{ RM_forincr( $7 ); }
		;
for_init	: asgn		{ $$ = $1; }
		| auto_lval	{ $$ = $1; }
		| empty 	{ $$ = $1; }
		;
for_test	: asgn		{ $$ = $1; }
		| expr		{ $$ = $1; }
		| empty		{ $$ = $1; }
		;
for_incr	: asgn		{ $$ = $1; }
		| auto_lval	{ $$ = $1; }
		| empty		{ $$ = $1; }
		;

asgn		: lval asgn_op asgn
				{ $$ = RM_node( $<ival>2, 0, $1, $3 );
				  if( rm_context == CTX_PARMS )
					PARM_add( $$ );
				  else if( rm_context == CTX_DESCR ||
					rm_context == CTX_SITES )
					SE_addval( $$ );
				}
		| lval asgn_op expr
				{ $$ = RM_node( $<ival>2, 0, $1, $3 );
				  if( rm_context == CTX_PARMS )
					PARM_add( $$ );
				  else if( rm_context == CTX_DESCR ||
					rm_context == CTX_SITES )
					SE_addval( $$ );
				}
		;
asgn_op		: SYM_ASSIGN	{ $<ival>$ = SYM_ASSIGN; }
		| SYM_MINUS_ASSIGN
				{ $<ival>$ = SYM_MINUS_ASSIGN; }
		| SYM_PLUS_ASSIGN
				{ $<ival>$ = SYM_PLUS_ASSIGN; }
		| SYM_PERCENT_ASSIGN
				{ $<ival>$ = SYM_PERCENT_ASSIGN; }
		| SYM_SLASH_ASSIGN
				{ $<ival>$ = SYM_SLASH_ASSIGN; }
		| SYM_STAR_ASSIGN
				{ $<ival>$ = SYM_STAR_ASSIGN; }
		;
expr		: conj		{ $$ = $1; }
		| expr SYM_OR conj
				{ $$ = RM_node( SYM_OR, 0, $1, $3 ); }
		;
conj		: compare	{ $$ = $1; }
		| compare SYM_AND conj
				{ $$ = RM_node( SYM_AND, 0, $1, $3 ); }
		;
compare		: site		{ $$ = $1; }
		| a_expr	{ $$ = $1; }
		| a_expr comp_op a_expr
				{ $$ = RM_node( $<ival>2, 0, $1, $3 ); }
		;
comp_op		: SYM_DONT_MATCH
				{ $<ival>$ = SYM_DONT_MATCH; }
		| SYM_EQUAL	{ $<ival>$ = SYM_EQUAL; }
		| SYM_GREATER	{ $<ival>$ = SYM_GREATER; }
		| SYM_GREATER_EQUAL
				{ $<ival>$ = SYM_GREATER_EQUAL; }
		| SYM_LESS	{ $<ival>$ = SYM_LESS; }
		| SYM_LESS_EQUAL
				{ $<ival>$ = SYM_LESS_EQUAL; }
		| SYM_MATCH	{ $<ival>$ = SYM_MATCH; }
		| SYM_NOT_EQUAL	{ $<ival>$ = SYM_NOT_EQUAL; }
		;
a_expr		: term		{ $$ = $1; }
		| a_expr add_op term
				{ $$ = RM_node( $<ival>2, 0, $1, $3 ); }
		;
add_op		: SYM_PLUS	{ $<ival>$ = SYM_PLUS; }
		| SYM_MINUS 	{ $<ival>$ = SYM_MINUS; }
		;
term		: factor	{ $$ = $1; }
		| term mul_op factor
				{ $$ = RM_node( $<ival>2, 0, $1, $3 ); }
		;
mul_op		: SYM_PERCENT	{ $<ival>$ = SYM_PERCENT; }
		| SYM_SLASH	{ $<ival>$ = SYM_SLASH; }
		| SYM_STAR	{ $<ival>$ = SYM_STAR; }
		;
factor		: primary	{ $$ = $1; }
		| SYM_MINUS primary
				{ $$ = RM_node( SYM_NEGATE, 0, 0, $2 ); }
		| SYM_NOT primary
				{ $$ = RM_node( SYM_NOT, 0, 0, $2 ); }
		| stref		{ if( rm_context == CTX_SCORE )
					$$ = $1;
				}
		;
pairing		: stref		{ if( rm_context == CTX_SCORE )
					$$ = $1;
				}
		| stref SYM_COLON pairing
				{ if( rm_context == CTX_SCORE )
					$$ = RM_node( SYM_COLON, 0, $1, $3 );
				}
		;
kw_pairing 	: kw_stref	{ if( rm_context == CTX_SCORE )
					$$ = $1;
				}
		| kw_stref SYM_COLON kw_pairing
				{ if( rm_context == CTX_SCORE )
					$$ = RM_node( SYM_COLON, 0, $1, $3 );
				}
		;
primary		: lval		{ $$ = $1; }
		| literal	{ $$ = $1; }
		| fcall		{ $$ = $1; }
		| SYM_LPAREN expr SYM_RPAREN
				{ $$ = $2; }
		;
fcall		: ident SYM_LPAREN e_list SYM_RPAREN
				{ $$ = RM_node( SYM_CALL, 0, $1, $3 ); }
		;
stref		: kw_stref	{ $$ = $1; }
		| ix_stref	{ $$ = $1; }
		;
kw_stref	: strhdr SYM_LPAREN a_list SYM_RPAREN
				{ if( rm_context == CTX_DESCR )
					SE_close();
				  else if( rm_context == CTX_SITES )
					POS_close();
				  else if( rm_context == CTX_SCORE )
					$$ = RM_node( SYM_KW_STREF, 0, $1, $3 );
				}
		;
ix_stref	: strhdr SYM_LBRACK e_list SYM_RBRACK
				{ $$ = RM_node( SYM_IX_STREF, 0, $1, $3 ); }
		;
lval		: ident		{ $$ = $1; }
		| auto_lval	{ $$ = $1; }
		;
auto_lval	: incr_op ident	{ $$ = RM_node( $<ival>1, 0, 0, $2 ); }
		| ident incr_op	{ $$ = RM_node( $<ival>2, 0, $1, 0 ); }
		;
literal		: SYM_INT	{ $$ = RM_node( SYM_INT, &rm_tokval, 0, 0 ); }
		| SYM_FLOAT	{ $$ = RM_node( SYM_FLOAT, &rm_tokval, 0, 0 ); }
		| SYM_DOLLAR	{ $$ = RM_node( SYM_DOLLAR,
					&rm_tokval, 0, 0 ); }
		| string	{ $$ = $1; }
		| pairset	{ $$ = $1; }
		;
ident		: SYM_IDENT 	{ $$ = RM_node( SYM_IDENT,
					&rm_tokval, 0, 0 ); }
		;
incr_op		: SYM_MINUS_MINUS
				{ $<ival>$ = SYM_MINUS_MINUS; }
		| SYM_PLUS_PLUS	{ $<ival>$ = SYM_PLUS_PLUS; }
		;
e_list		: expr		{ $$ = RM_node( SYM_LIST, 0, $1, 0 ); }
		| expr SYM_COMMA e_list
				{ $$ = RM_node( SYM_LIST, 0, $1, $3 ); }
		;
a_list		: asgn		{ if( rm_context == CTX_SCORE )
					$$ = RM_node( SYM_LIST, 0, $1, 0 );
				}
		| asgn SYM_COMMA a_list
				{ if( rm_context == CTX_SCORE )
					$$ = RM_node( SYM_LIST, 0, $1, $3 );
				}
		;
pairset		: SYM_LCURLY 	{ PR_open(); }
			s_list SYM_RCURLY
				{ $$ = PR_close(); }
		;
s_list		: string	{ PR_add( $$ ); }
		| string SYM_COMMA s_list
				{ PR_add( $1 ) ; }
		;
string		: SYM_STRING	{ $$ = RM_node( SYM_STRING,
					&rm_tokval, 0, 0 ); }
		;
empty		: 		{ $$ = NULL; }
		;
%%

#include "lex.yy.c"

int	yyerror( msg )
char	msg[];
{

	fprintf( stderr, "yyerror: %s\n", msg );
	return( 0 );
}
