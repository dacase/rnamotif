#include <stdio.h>
#include <string.h>
#include "rnamot.h"
#include "y.tab.h"

extern	int	rm_emsg_lineno;
extern	int	rm_dminlen;
extern	int	rm_dmaxlen;

static	char	fm_emsg[ 256 ];
static	char	*fm_locus;
static	char	fm_slen;
static	char	*fm_sbuf;
static	int	*fm_window;
static	int	fm_windowsize;

static	int	find_motif();
static	int	find_ss();
static	int	find_wchlx();
static	int	find_pknot();

IDENT_T	*find_id();

int	find_motif_driver( n_descr, descr, sites, locus, slen, sbuf )
int	n_descr;
STREL_T descr[];
SITE_T	*sites;
char	locus[];
int	slen;
char	sbuf[];
{
	int	i, w_winsize, s_zero, s_dollar;
	IDENT_T	*ip;
	
	if( fm_window == NULL ){
		ip = find_id( "windowsize" );
		if( ip == NULL )
			errormsg( 1,
				"find_motif_driver: windowsize undefined." );
	
			if( ip->i_val.v_value.v_ival <= 0 )
				errormsg( 1,
					"find_motif_driver: windowsize <= 0." );
		else
			fm_windowsize = ip->i_val.v_value.v_ival;
		fm_window = ( int * )malloc( fm_windowsize * sizeof( int ) );
		if( fm_window == NULL )
			errormsg( 1,
				"find_motif_driver: can't allocate fm_window.");
	}

fprintf( stderr, "fmd: locus = %s, slen = %d\n", locus, slen );

	fm_locus = locus;
	fm_slen = slen;
	fm_sbuf = sbuf;

	w_winsize = rm_dmaxlen < fm_windowsize ? rm_dmaxlen : fm_windowsize;

	for( s_zero = 0; s_zero < slen - w_winsize; s_zero++ ){
		s_dollar = s_zero + w_winsize - 1;
fprintf( stderr, "srch.1: 0, %4d:%4d, %4d\n", s_zero, s_dollar, slen - 1 );
/*
		find_motif( 0, descr, i, i + rm_dmaxlen - 1 );
*/
	}

	s_dollar = slen - 1;
	for( ; s_zero <= slen - rm_dminlen; s_zero++ ){
fprintf( stderr, "srch.2: 0, %4d:%4d, %4d\n", s_zero, s_dollar, slen - 1);
/*
		find_motif( 0, descr, i, i + rm_dmaxlen - 1 );
*/
	}

	return( 0 );
}

static	int	find_motif( d, descr, s_zero, s_dollar )
int	d;
STREL_T	descr[];
int	s_zero;
int	s_dollar;
{
	STREL_T	*stp;

	stp = &descr[ d ];
	switch( stp->s_type ){
	case SYM_SS :
		find_ss( d, descr, s_zero, s_dollar );
		break;
	case SYM_H5 :
		if( stp->s_proper )
			find_wchlx( d, descr, s_zero, s_dollar );
		else
			find_pknot( d, descr, s_zero, s_dollar );
		break;
	case SYM_P5 :
		rm_emsg_lineno = stp->s_lineno;
		errormsg( 1, "parallel helix finder not implemented." );
		break;
	case SYM_T1 :
		rm_emsg_lineno = stp->s_lineno;
		errormsg( 1, "triple helix finder not implemented." );
		break;
	case SYM_Q1 :
		rm_emsg_lineno = stp->s_lineno;
		errormsg( 1, "quad helix finder not implemented." );
		break;
	case SYM_H3 :
	case SYM_P3 :
	case SYM_T2 :
	case SYM_T3 :
	case SYM_Q2 :
	case SYM_Q3 :
	case SYM_Q4 :
	default :
		rm_emsg_lineno = stp->s_lineno;
		sprintf( fm_emsg, "find_motif: illegal symbol %d.",
			stp->s_type );
		errormsg( 1, fm_emsg );
		break;
	}
}

static	int	find_ss( d, descr, s_zero, s_dollar )
int	d;
STREL_T	descr[];
int	s_zero;
int	s_dollar;
{

}

static	int	find_wchlx( d, descr, s_zero, s_dollar )
int	d;
STREL_T	descr[];
int	s_zero;
int	s_dollar;
{

}

static	int	find_pknot( d, descr, s_zero, s_dollar )
int	d;
STREL_T	descr[];
int	s_zero;
int	s_dollar;
{

}
