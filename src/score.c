#include <stdio.h>

#include "rnamot.h"
#include "y.tab.h"

#define	ISLVAL(s)	\
	((s)==SYM_ASSIGN||(s)==SYM_PLUS_ASSIGN||(s)==SYM_MINUS_ASSIGN||\
	 (s)==SYM_PERCENT_ASSIGN||(s)==SYM_SLASH_ASSIGN||(s)==SYM_STAR_ASSIGN||\
	 (s)==SYM_PLUS_PLUS||(s)==SYM_MINUS_MINUS)

#define	ISRFLX(s)	\
	((s)==SYM_PLUS_ASSIGN||(s)==SYM_MINUS_ASSIGN||\
	 (s)==SYM_PERCENT_ASSIGN||(s)==SYM_SLASH_ASSIGN||(s)==SYM_STAR_ASSIGN)

#define	CFP	stdout

extern	int	rm_lineno;

static	int	nextlab = 1;

static	int	actlab;

static	int	ifstk[ 100 ];
static	int	ifstkp = 0;

static	int	loopstk[ 100 ];
static	NODE_T	*loopincrstk[ 100 ];
static	int	loopstkp = 0;

void	SC_if();
void	SC_else();
void	SC_endelse();
void	SC_endif();
void	SC_forinit();
void	SC_fortest();
void	SC_forincr();
void	SC_endfor();
void	SC_while();
void	SC_endwhile();
void	SC_break();
void	SC_continue();
void	SC_accept();
void	SC_reject();
void	SC_mark();
void	SC_clear();
void	SC_expr();
void	SC_node();
static	void	mk_stref_name();

void	SC_action( np )
NODE_T	*np;
{

	SC_mark();
	SC_expr( 0, np );
	actlab = nextlab;
	nextlab++;
	fprintf( CFP, "  fjp L%d\n", actlab );
}

void	SC_endaction()
{

	fprintf( CFP, "L%d:\n", actlab );
}

void	SC_if( np )
NODE_T	*np;
{

	SC_mark();
	SC_expr( 0, np );
	ifstk[ ifstkp ] = nextlab;
	ifstkp++;
	nextlab += 2;
	fprintf( CFP, "  fjp L%d\n", ifstk[ ifstkp - 1 ] );
}

void	SC_else()
{

	fprintf( CFP, "  jmp L%d\n", ifstk[ ifstkp - 1 ] + 1 );
	fprintf( CFP, "L%d:\n", ifstk[ ifstkp - 1 ] );
}

void	SC_endelse()
{

	fprintf( CFP, "L%d:\n", ifstk[ ifstkp - 1 ] + 1 );
	ifstkp--;
}

void	SC_endif()
{

	fprintf( CFP, "L%d:\n", ifstk[ ifstkp - 1 ] );
	ifstkp--;
}

void	SC_forinit( np )
NODE_T	*np;
{

	loopstk[ loopstkp ] = nextlab;
	loopstkp++;
	nextlab += 3;
	SC_mark();
	SC_expr( 0, np );
	SC_clear();
}

void	SC_fortest( np )
NODE_T	*np;
{

	fprintf( CFP, "L%d:\n", loopstk[ loopstkp - 1 ] );
	SC_mark();
	SC_expr( 0, np );
	fprintf( CFP, "  fjp L%d\n", loopstk[ loopstkp - 1 ] + 2 );
}

void	SC_forincr( np )
NODE_T	*np;
{

	loopincrstk[ loopstkp - 1 ] = np;
}

void	SC_endfor()
{

	fprintf( CFP, "L%d:\n", loopstk[ loopstkp - 1 ] + 1 );
	SC_mark();
	SC_expr( 0, loopincrstk[ loopstkp - 1 ] );
	SC_clear();
	fprintf( CFP, "  jmp L%d\n", loopstk[ loopstkp - 1 ] );
	fprintf( CFP, "L%d:\n", loopstk[ loopstkp - 1 ] + 2 );
	loopstkp--;
}

void	SC_while( np )
NODE_T	*np;
{

	loopstk[ loopstkp ] = nextlab;
	loopstkp++;
	nextlab += 3;
	fprintf( CFP, "L%d:\n", loopstk[ loopstkp - 1 ] );
	SC_mark();
	SC_expr( 0, np );
	fprintf( CFP, "  fjp L%d\n", loopstk[ loopstkp - 1 ] + 2 );
}

void	SC_endwhile()
{

	fprintf( CFP, "  jmp L%d\n", loopstk[ loopstkp - 1 ] );
	fprintf( CFP, "L%d:\n", loopstk[ loopstkp - 1 ] + 2 );
	loopstkp--;
}

void	SC_break()
{

	fprintf( CFP, "  jmp L%d\n", loopstk[ loopstkp - 1 ] + 1 );
}

void	SC_continue()
{

	fprintf( CFP, "  jmp L%d\n", loopstk[ loopstkp - 1 ] );
}

void	SC_accept()
{

	fprintf( CFP, "  ACCEPT\n" );
}

void	SC_reject()
{

	fprintf( CFP, "  REJECT\n" );
}

void	SC_mark()
{

	fprintf( CFP, "  mrk\n" );
}

void	SC_clear()
{

	fprintf( CFP, "  clr\n" );
}

void	SC_expr( lval, np )
int	lval;
NODE_T	*np;
{
	char	name[ 20 ];

	if( np ){
		if( np->n_sym == SYM_CALL )
			fprintf( CFP, "  mrk\n" );
		else if( np->n_sym == SYM_STREF ){
			mk_stref_name( np->n_left->n_sym, name );
			fprintf( CFP, "  strf %s\n", name );
		}else if( np->n_sym == SYM_LCURLY )
			fprintf( CFP, "  mrk\n" );
		SC_expr( ISLVAL( np->n_sym ), np->n_left );
		if( ISRFLX( np->n_sym ) )
			SC_expr( 0, np->n_left );
		SC_expr( 0, np->n_right );
		SC_node( lval, np );
		if( np->n_sym == SYM_STREF ){
			fprintf( CFP, "  strf cls\n" );
		}else if( np->n_sym == SYM_LCURLY )
			fprintf( CFP, "  prst\n" );
	}
}

void	SC_node( lval, np )
int	lval;
NODE_T	*np;
{
	POS_T	*posp;

	switch( np->n_sym ){

	case SYM_CALL :
		fprintf( CFP, "  fcl %s\n", np->n_val.v_value.v_pval );
		break;
	case SYM_LIST :
		break;
	case SYM_STREF :
		break;

	case SYM_FLOAT :
		fprintf( CFP, "  ldcR\n" );
		break;

	case SYM_PARMS :
		break;
	case SYM_DESCR :
		break;
	case SYM_SITES :
		break;

	case SYM_SS :
	case SYM_H5 :
	case SYM_H3 :
	case SYM_P5 :
	case SYM_P3 :
	case SYM_T1 :
	case SYM_T2 :
	case SYM_T3 :
	case SYM_Q1 :
	case SYM_Q2 :
	case SYM_Q3 :
	case SYM_Q4 :
		break;

	case SYM_ELSE :
		break;
	case SYM_FOR :
		break;
	case SYM_IF :
		break;
	case SYM_IN :
		fprintf( CFP, "  ins\n" );
		break;
	case SYM_WHILE :
		break;
	case SYM_IDENT :
		if( lval )
			fprintf( CFP, "  lda %s\n", np->n_val.v_value.v_pval );
		else
			fprintf( CFP, "  lod %s\n", np->n_val.v_value.v_pval );
		break;
	case SYM_INT :
		fprintf( CFP, "  ldc %d\n", np->n_val.v_value.v_ival );
		break;
	case SYM_STRING :
		fprintf( CFP, "  ldc \"%s\"\n", np->n_val.v_value.v_pval );
		break;
	case SYM_AND :
		fprintf( CFP, "  and\n" );
		break;
	case SYM_ASSIGN :
		fprintf( CFP, "  sto\n" );
		break;
	case SYM_PLUS_ASSIGN :
		fprintf( CFP, "  add\n" );
		fprintf( CFP, "  sto\n" );
		break;
	case SYM_MINUS_ASSIGN :
		fprintf( CFP, "  sub\n" );
		fprintf( CFP, "  sto\n" );
		break;
	case SYM_PLUS :
		fprintf( CFP, "  add\n" );
		break;
	case SYM_MINUS :
		fprintf( CFP, "  sub\n" );
		break;
	case SYM_DOLLAR :
		fprintf( CFP, "  lod $\n" );
		break;
	case SYM_DONT_MATCH :
		break;
	case SYM_EQUAL :
		fprintf( CFP, "  equ\n" );
		break;
	case SYM_GREATER :
		fprintf( CFP, "  gtr\n" );
		break;
	case SYM_GREATER_EQUAL :
		fprintf( CFP, "  geq\n" );
		break;
	case SYM_LESS :
		fprintf( CFP, "  les\n" );
		break;
	case SYM_LESS_EQUAL :
		fprintf( CFP, "  leq\n" );
		break;
	case SYM_MATCH :
		fprintf( CFP, "  mat\n" );
		break;
	case SYM_MINUS_MINUS :
		if( np->n_left )
			fprintf( CFP, "  dcp\n" );
		else
			fprintf( CFP, "  pdc\n" );
		break;
	case SYM_NEGATE :
		fprintf( CFP, "  neg\n" );
		break;
	case SYM_NOT :
		fprintf( CFP, "  not\n" );
		break;
	case SYM_NOT_EQUAL :
		fprintf( CFP, "  neq\n" );
		break;
	case SYM_OR :
		fprintf( CFP, "  ior\n" );
		break;
	case SYM_PERCENT :
		fprintf( CFP, "  mod\n" );
		break;
	case SYM_PERCENT_ASSIGN :
		fprintf( CFP, "  mod\n" );
		fprintf( CFP, "  sto\n" );
		break;
	case SYM_PLUS_PLUS :
		if( np->n_left )
			fprintf( CFP, "  icp\n" );
		else
			fprintf( CFP, "  pic\n" );
		break;
	case SYM_STAR :
		fprintf( CFP, "  mul\n" );
		break;
	case SYM_STAR_ASSIGN :
		fprintf( CFP, "  mul\n" );
		fprintf( CFP, "  sto\n" );
		break;
	case SYM_SLASH :
		fprintf( CFP, "  div\n" );
		break;
	case SYM_SLASH_ASSIGN :
		fprintf( CFP, "  div\n" );
		fprintf( CFP, "  sto\n" );
		break;
	case SYM_LPAREN :
		break;
	case SYM_RPAREN :
		break;
	case SYM_LCURLY :
		break;
	case SYM_RCURLY :
		break;
	case SYM_COLON :
		fprintf( CFP, "  bpr\n" );
		break;
	case SYM_COMMA :
		break;
	case SYM_SEMICOLON :
		break;
	case SYM_ERROR :
		fprintf( CFP, "SYM_ERROR\n" );
		break;
	default :
		fprintf( stderr, "SC_node: Unknown symbol %d\n", np->n_sym );
		break;

	}
}

static	void	mk_stref_name( sym, name )
int	sym;
char	name[];
{

	switch( sym ){
	case SYM_SS :
		strcpy( name, "ss" );
		break;

	case SYM_H5 :
		strcpy( name, "h5" );
		break;
	case SYM_H3 :
		strcpy( name, "h3" );
		break;

	case SYM_P5 :
		strcpy( name, "p5" );
		break;
	case SYM_P3 :
		strcpy( name, "p3" );
		break;

	case SYM_T1 :
		strcpy( name, "t1" );
		break;
	case SYM_T2 :
		strcpy( name, "t2" );
		break;
	case SYM_T3 :
		strcpy( name, "t3" );
		break;

	case SYM_Q1 :
		strcpy( name, "q1" );
		break;
	case SYM_Q2 :
		strcpy( name, "q2" );
		break;
	case SYM_Q3 :
		strcpy( name, "q3" );
		break;
	case SYM_Q4 :
		strcpy( name, "q4" );
		break;
	}
}
