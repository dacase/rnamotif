#include <stdio.h>

#include "rnamot.h"
#include "y.tab.h"

extern	int	rmlineno;

void	dumpnode();

NODE_T	*node( sym, vp, left, right )
int	sym;
VALUE_T	*vp;
NODE_T	*left;
NODE_T	*right;
{
	NODE_T	*np;

	np = ( NODE_T * )malloc( sizeof( NODE_T ) );
	if( np == NULL ){
		fprintf( stderr, "node: FATAL: can't alloc np for sym = %d.\n",
			sym );
		exit( 1 );
	}
	np->n_sym = sym;
	np->n_type = T_UNDEF;
	np->n_class = C_UNDEF;
	np->n_lineno = rmlineno;
	np->n_left = left;
	np->n_right = right;
	if( sym == SYM_IDENT ){
		np->n_val.v_type = T_STRING;
		np->n_val.v_value.v_pval = vp->v_value.v_pval;
	}else if( sym == SYM_INT ){
		np->n_val.v_type = T_INT;
		np->n_val.v_value.v_ival = vp->v_value.v_ival;
	}else if( sym == SYM_STRING ){
		np->n_val.v_type = T_STRING;
		np->n_val.v_value.v_pval = vp->v_value.v_pval;
	}
	return( np );
}

void	dumpexpr( fp, np, indent )
FILE	*fp;
NODE_T	*np;
int	indent;
{

	if( np ){
		dumpnode( fp, np, indent );
		dumpexpr( fp, np->n_left, indent + 3 );
		dumpexpr( fp, np->n_right, indent + 3 );
	}
}

void	dumpnode( fp, np, indent )
FILE	*fp;
NODE_T	*np;
int	indent;
{

	fprintf( fp, "%*s",indent, "" );
	fprintf( fp, "%7d: lf = %7d rt = %7d tp,cl = %2d,%2d ",
		np, np->n_left, np->n_right, np->n_type, np->n_class );
	switch( np->n_sym ){
	case SYM_PARMS :
		fprintf( fp, "SYM_PARMS\n" );
		break;
	case SYM_DESCR :
		fprintf( fp, "SYM_DESCR\n" );
		break;
	case SYM_SITES :
		fprintf( fp, "SYM_SITES\n" );
		break;
	case SYM_H5 :
		fprintf( fp, "SYM_H5\n" );
		break;
	case SYM_H3 :
		fprintf( fp, "SYM_H3\n" );
		break;
	case SYM_P5 :
		fprintf( fp, "SYM_P5\n" );
		break;
	case SYM_P3 :
		fprintf( fp, "SYM_P3\n" );
		break;
	case SYM_T1 :
		fprintf( fp, "SYM_T1\n" );
		break;
	case SYM_T2 :
		fprintf( fp, "SYM_T2\n" );
		break;
	case SYM_T3 :
		fprintf( fp, "SYM_T3\n" );
		break;
	case SYM_Q1 :
		fprintf( fp, "SYM_Q1\n" );
		break;
	case SYM_Q2 :
		fprintf( fp, "SYM_Q2\n" );
		break;
	case SYM_Q3 :
		fprintf( fp, "SYM_Q3\n" );
		break;
	case SYM_Q4 :
		fprintf( fp, "SYM_Q4\n" );
		break;
	case SYM_IDENT :
		fprintf( fp, "SYM_IDENT = '%s'\n", np->n_val.v_value.v_pval );
		break;
	case SYM_INT :
		fprintf( fp, "SYM_INT = %d\n", np->n_val.v_value.v_ival );
		break;
	case SYM_STRING :
		fprintf( fp, "SYM_STRING = '%s'\n", np->n_val.v_value.v_pval );
		break;
	case SYM_ASSIGN :
		fprintf( fp, "SYM_ASSIGN\n" );
		break;
	case SYM_PLUS_ASSIGN :
		fprintf( fp, "SYM_PLUS_ASSIGN\n" );
		break;
	case SYM_MINUS_ASSIGN :
		fprintf( fp, "SYM_MINUS_ASSIGN\n" );
		break;
	case SYM_PLUS :
		fprintf( fp, "SYM_PLUS\n" );
		break;
	case SYM_MINUS :
		fprintf( fp, "SYM_MINUS\n" );
		break;
	case SYM_DOLLAR :
		fprintf( fp, "SYM_DOLLAR\n" );
		break;
	case SYM_LPAREN :
		fprintf( fp, "SYM_LPAREN\n" );
		break;
	case SYM_RPAREN :
		fprintf( fp, "SYM_RPAREN\n" );
		break;
	case SYM_LCURLY :
		fprintf( fp, "SYM_LCURLY\n" );
		break;
	case SYM_RCURLY :
		fprintf( fp, "SYM_RCURLY\n" );
		break;
	case SYM_COMMA :
		fprintf( fp, "SYM_COMMA\n" );
		break;
	case SYM_COLON :
		fprintf( fp, "SYM_COLON\n" );
		break;
	case SYM_ERROR :
		fprintf( fp, "SYM_ERROR\n" );
		break;
	}
}
