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

static	int	find_motif();
static	int	find_ss();
static	int	find_wchlx();
static	int	find_pknot();

int	find_motif_driver( n_descr, descr, sites, locus, slen, sbuf )
int	n_descr;
STREL_T descr[];
SITE_T	*sites;
char	locus[];
int	slen;
char	sbuf[];
{
	int	i;
	
fprintf( stderr, "fmd: locus = %s, slen = %d\n", locus, slen );

	fm_locus = locus;
	fm_slen = slen;
	fm_sbuf = sbuf;

	for( i = 0; i < slen - rm_dminlen; i++ ){
		find_motif( 0, descr, i, i + rm_dmaxlen - 1 );
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
