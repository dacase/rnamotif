#include <stdio.h>

#include "rnamot.h"
#include "y.tab.h"

extern	int	rm_lineno;

void	dumpnode();

NODE_T	*node( sym, vp, left, right )
int	sym;
VALUE_T	*vp;
NODE_T	*left;
NODE_T	*right;
{
	NODE_T	*np;
	char	emsg[ 256 ];

	np = ( NODE_T * )malloc( sizeof( NODE_T ) );
	if( np == NULL ){
		sprintf( emsg, "node: can't allocate np for sym %d.", sym );
		RM_errormsg( 1, emsg );
	}
	np->n_sym = sym;
	np->n_type = T_UNDEF;
	np->n_class = C_UNDEF;
	np->n_lineno = rm_lineno;
	np->n_left = left;
	np->n_right = right;
	if( sym == SYM_IDENT ){
		np->n_val.v_type = T_IDENT;
		np->n_val.v_value.v_pval = vp->v_value.v_pval;
	}else if( sym == SYM_INT ){
		np->n_class = C_LIT;
		np->n_val.v_type = T_INT;
		np->n_val.v_value.v_ival = vp->v_value.v_ival;
	}else if( sym == SYM_FLOAT ){
		np->n_class = C_LIT;
		np->n_val.v_type = T_FLOAT;
		np->n_val.v_value.v_fval = vp->v_value.v_fval;
	}else if( sym == SYM_STRING ){
		np->n_class = C_LIT;
		np->n_val.v_type = T_STRING;
		np->n_val.v_value.v_pval = vp->v_value.v_pval;
	}else if( sym == SYM_DOLLAR ){
		np->n_class = C_LIT;
		np->n_val.v_type = T_POS;
		np->n_val.v_value.v_pval = vp->v_value.v_pval;
	}else if( sym == SYM_CALL ){
		np->n_val.v_type = T_IDENT;
		np->n_val.v_value.v_pval = left->n_val.v_value.v_pval;
		np->n_left = NULL;
	}
	return( np );
}

NODE_T	*updnode( np, vp, left, right )
NODE_T	*np;
VALUE_T	*vp;
NODE_T	*left;
NODE_T	*right;
{

	if( left != NULL )
		np->n_left = left;
	if( right != NULL )
		np->n_right = right;
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
	POS_T	*posp;

	fprintf( fp, "%*s",indent, "" );
	fprintf( fp, "%7p: lf = %7p rt = %7p ln,tp,cl = %3d,%2d,%2d ",
		np, np->n_left, np->n_right,
		np->n_lineno, np->n_type, np->n_class );
	switch( np->n_sym ){

#include "dumpnode.h"

	}
}
