#include <stdio.h>
#include <stdlib.h>

#include "log.h"
#include "rmdefs.h"
#include "rnamot.h"
#include "y.tab.h"

extern	int	rm_error;
extern	char	*rm_wdfname;
extern	int	rm_lineno;

NODE_T	*RM_node( int sym, VALUE_T *vp, NODE_T *left, NODE_T *right )
{
	NODE_T	*np;

	np = ( NODE_T * )malloc( sizeof( NODE_T ) );
	if( np == NULL ){
		rm_error = TRUE;
		LOG_ERROR("%s:%d can't allocate np for sym %d", rm_wdfname, rm_lineno, sym);
		exit(1);
	}
	np->n_sym = sym;
	np->n_type = T_UNDEF;
	np->n_class = C_UNDEF;
	np->n_filename = rm_wdfname;
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
		np->n_val.v_value.v_dval = vp->v_value.v_dval;
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

void	RM_dumpexpr( FILE *fp, NODE_T *np, int indent )
{

	if( np ){
		RM_dumpnode( fp, np, indent );
		RM_dumpexpr( fp, np->n_left, indent + 3 );
		RM_dumpexpr( fp, np->n_right, indent + 3 );
	}
}

void	RM_dumpnode( FILE *fp, NODE_T *np, int indent )
{

	fprintf( fp, "%*s",indent, "" );
	fprintf( fp, "%8p: lf = %8p rt = %8p fn,ln,tp,cl = %s:%3d,%2d,%2d ",
		np, np->n_left, np->n_right,
		np->n_filename, np->n_lineno, np->n_type, np->n_class );
	switch( np->n_sym ){

#include "dumpnode.h"

	}
}
