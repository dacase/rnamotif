#include <stdio.h>
#include <string.h>

#include "rnamot.h"
#include "y.tab.h"

#define	VALSTKSIZE	20
static	VALUE_T	valstk[ VALSTKSIZE ];
static	int	n_valstk;

static	STREL_T	*stp = NULL;

void	SE_dump();

void	SE_new( stype )
int	stype;
{

	n_valstk = 0;
	stp = ( STREL_T * )malloc( sizeof( STREL_T ) );
	if( stp == NULL ){
		fprintf( stderr, "SE_new: FATAL: can't alloc stp.\n" );
		exit( 1 );
	}
	stp->s_type = stype;
	stp->s_tag = NULL;
	stp->s_next = NULL;
	stp->s_pairs = NULL;
	stp->s_minlen = 0;
	stp->s_maxlen = 0;
	stp->s_seq = NULL;
	stp->s_mismatch = 0;
	stp->s_mispair = 0;
	stp->s_pairdata = NULL;
	stp->s_sites = NULL;
}

void	SE_saveval( vp )
VALUE_T	*vp;
{

	if( n_valstk == VALSTKSIZE ){
		fprintf( stderr, "SE_saveval: FATAL: valstk overflow.\n" );
		exit( 1 );
	}
	valstk[ n_valstk ].v_sym = vp->v_sym;
	switch( vp->v_sym ){
	case SYM_INT :
		valstk[ n_valstk ].v_sym = SYM_INT;
		valstk[ n_valstk ].v_value.v_ival = vp->v_value.v_ival;
		break;
	case SYM_DOLLAR :
		valstk[ n_valstk ].v_sym = SYM_DOLLAR;
		valstk[ n_valstk ].v_value.v_ival = 0; 
		break;
	case SYM_STRING :
		valstk[ n_valstk ].v_sym = SYM_STRING;
		valstk[ n_valstk ].v_value.v_cval = vp->v_value.v_cval;
		break;
	case SYM_IDENT :
		valstk[ n_valstk ].v_sym = SYM_IDENT;
		valstk[ n_valstk ].v_value.v_pval = vp->v_value.v_cval;
		break;
	default :
		fprintf( stderr,
			"SE_saveval: FATAL: unkwnown value sym %d.\n",
			vp->v_sym );
		exit( 1 );
		break;
	}
	n_valstk++;
}

void	SE_addtag( vp )
VALUE_T	*vp;
{
	char	*sp;

	if( vp->v_sym == SYM_IDENT )
		stp->s_tag = vp->v_value.v_cval;
	else{
		fprintf( stderr, "SE_addtag: Unknown value symbol %d.\n",
			vp->v_sym );
		exit( 1 );
	}
}

void	SE_addlen()
{

	n_valstk = 0;
}

void	SE_addseq()
{

	stp->s_seq = valstk[ n_valstk - 1 ].v_value.v_cval;
	n_valstk = 0;
}

void	SE_close()
{

	SE_dump( stderr, stp );
}

void	SE_dump( fp, stp )
FILE	*fp;
STREL_T	*stp;
{

	fprintf( fp, "stp->s_type = " );
	switch( stp->s_type ){
	case SYM_SS :
		fprintf( fp, "ss" );
		break;
	case SYM_H5 :
		fprintf( fp, "h5" );
		break;
	case SYM_H3 :
		fprintf( fp, "h3" );
		break;
	case SYM_P5 :
		fprintf( fp, "p5" );
		break;
	case SYM_P3 :
		fprintf( fp, "p3" );
		break;
	case SYM_T1 :
		fprintf( fp, "t1" );
		break;
	case SYM_T2 :
		fprintf( fp, "t2" );
		break;
	case SYM_T3 :
		fprintf( fp, "t3" );
		break;
	case SYM_Q1 :
		fprintf( fp, "q1" );
		break;
	case SYM_Q2 :
		fprintf( fp, "q2" );
		break;
	case SYM_Q3 :
		fprintf( fp, "q3" );
		break;
	case SYM_Q4 :
		fprintf( fp, "q4" );
		break;
	default :
		fprintf( fp, "unknown (%d)", stp->s_type );
		break;
	}
	fprintf( fp, "\n" );

	fprintf( fp, "stp->s_tag  = %s\n",
		stp->s_tag ? stp->s_tag : "(No tag)" );

	fprintf( fp, "stp->s_seq  = %s\n",
		stp->s_seq ? stp->s_seq : "(No seq)" );
}
