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

int	find_motif_driver( n_searches, searches, sites, locus, slen, sbuf )
int	n_searches;
SEARCH_T	*searches[];
SITE_T	*sites;
char	locus[];
int	slen;
char	sbuf[];
{
	int	i, w_winsize, szero, sdollar;
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

	for( szero = 0; szero < slen - w_winsize; szero++ ){
		sdollar = szero + w_winsize - 1;
		find_motif( 0, n_searches, searches, szero, sdollar, slen );
	}

	sdollar = slen - 1;
	for( ; szero <= slen - rm_dminlen; szero++ ){
		find_motif( 0, n_searches, searches, szero, sdollar, slen );
	}

	return( 0 );
}

static	int	find_motif( slev, n_searches, searches, szero, sdollar, slen )
int	szero;
int	n_searches;
SEARCH_T	*searches[];
int	sdollar;
int	slen;
{
	SEARCH_T	*srp;
	STREL_T	*stp;

	srp = searches[ slev ];
	stp = srp->s_descr;

fprintf( stderr, "fm: slev = %d, str = 0, %4d:%4d, %4d\n",
	slev, szero, sdollar, slen - 1 );

	return(1);

	switch( stp->s_type ){
	case SYM_SS :
		find_ss( slev, n_searches, searches, szero, sdollar, slen );
		break;
	case SYM_H5 :
		if( stp->s_proper ){
			find_wchlx( slev, n_searches, searches,
				szero, sdollar, slen );
		}else{
			find_pknot(  slev, n_searches, searches,
				szero, sdollar, slen );
		}
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

static	int	find_ss( slev, n_searches, searches, szero, sdollar, slen )
int	slev;
int	n_searches;
SEARCH_T	*searches[];
int	szero;
int	sdollar;
int	slen;
{

}

static	int	find_wchlx( slev, n_searches, searches, szero, sdollar, slen )
int	slev;
int	n_searches;
SEARCH_T	*searches[];
int	szero;
int	sdollar;
int	slen;
{

}

static	int	find_pknot(  slev, n_searches, searches, szero, sdollar, slen )
int	slev;
int	n_searches;
SEARCH_T	*searches[];
int	szero;
int	sdollar;
int	slen;
{

}
