#include <stdio.h>
#include <string.h>
#include "rnamot.h"
#include "y.tab.h"

#define	MIN(a,b)	((a)<(b)?(a):(b))
#define	MAX(a,b)	((a)>(b)?(a):(b))

extern	int	rm_emsg_lineno;
extern	int	rm_dminlen;
extern	int	rm_dmaxlen;
extern	int	rm_b2bc[];

static	char	fm_emsg[ 256 ];
static	char	*fm_locus;
static	int	fm_slen;
static	char	*fm_sbuf;
static	int	*fm_window;
static	int	fm_windowsize;

static	int	find_motif();
static	int	find_ss();
static	int	find_wchlx();
static	int	find_pknot();
static	int	match_helix();
static	int	paired();

IDENT_T	*find_id();

int	find_motif_driver( n_searches, searches, sites, locus, slen, sbuf )
int	n_searches;
SEARCH_T	*searches[];
SITE_T	*sites;
char	locus[];
int	slen;
char	sbuf[];
{
	int	i, w_winsize, slev, szero, sdollar;
	IDENT_T	*ip;
	SEARCH_T	*srp;
	
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

	slev = 0;
	srp = searches[ slev ];
	for( szero = 0; szero < slen - w_winsize; szero++ ){
		sdollar = szero + w_winsize - 1;
		srp->s_zero = szero;
		srp->s_dollar = sdollar;
		find_motif( slev, n_searches, searches, szero, sdollar );
	}

	sdollar = slen - 1;
	for( ; szero <= slen - rm_dminlen; szero++ ){
		srp->s_zero = szero;
		srp->s_dollar = sdollar;
		find_motif( 0, n_searches, searches, szero, sdollar );
	}

	return( 0 );
}

static	int	find_motif( slev, n_searches, searches, szero, sdollar )
int	szero;
int	n_searches;
SEARCH_T	*searches[];
int	sdollar;
{
	SEARCH_T	*srp;
	STREL_T	*stp;

	srp = searches[ slev ];
	stp = srp->s_descr;

/*
fprintf( stderr, "fm: slev = %d, str = 0, %4d:%4d, %4d\n",
	slev, szero, sdollar, fm_slen - 1 );
*/

	switch( stp->s_type ){
	case SYM_SS :
		find_ss( slev, n_searches, searches, szero, sdollar );
		break;
	case SYM_H5 :
		if( stp->s_proper ){
			find_wchlx( slev, n_searches, searches,
				szero, sdollar );
		}else{
			find_pknot(  slev, n_searches, searches,
				szero, sdollar );
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

static	int	find_ss( slev, n_searches, searches, szero, sdollar )
int	slev;
int	n_searches;
SEARCH_T	*searches[];
int	szero;
int	sdollar;
{

}

static	int	find_wchlx( slev, n_searches, searches, szero, sdollar )
int	slev;
int	n_searches;
SEARCH_T	*searches[];
int	szero;
int	sdollar;
{
	SEARCH_T	*srp;
	STREL_T	*stp;
	int	s, s3lim, slen;
	int	h_minl, h_maxl;
	int	i_minl;

/*
fprintf( stderr, "fwch:\n" );
*/

	srp = searches[ slev ];
	stp = srp->s_descr;
	slen = sdollar - szero + 1;
	h_minl = stp->s_minlen;
	h_maxl = stp->s_maxlen;
	i_minl = stp->s_minilen;

/*
fprintf( stderr, "fwch: szero  = %4d, sdollar = %4d\n", szero, sdollar );
fprintf( stderr, "fwch: slen   = %4d\n", slen );
fprintf( stderr, "fwch: h_minl = %4d, h_maxl  = %4d\n", h_minl, h_maxl );
fprintf( stderr, "fwch: i_minl = %4d\n", i_minl );
*/

	s3lim = ( i_minl + 2 * h_minl );

/*
fprintf( stderr, "fwch: s3lim = ( i_minl + 2 * h_minl ) = %d\n", s3lim );
*/

	s3lim = szero + s3lim - 1;

/*
fprintf( stderr, "fwch: s3lim = szero + s3lim - 1 = %d\n", s3lim );
*/

	for( s = sdollar; s >= s3lim; s-- ){
		if( match_helix( stp, szero, s, s3lim ) ){
		}
	}
}

static	int	find_pknot(  slev, n_searches, searches, szero, sdollar )
int	slev;
int	n_searches;
SEARCH_T	*searches[];
int	szero;
int	sdollar;
{

}
static	int	match_helix( stp, s5, s3, s3lim )
STREL_T	*stp;
int	s5;
int	s3;
int	s3lim;
{
	int	s, s3_5plim;
	int	b5, b3;
	int	bpcnt, hlen, mpr;

	b5 = fm_sbuf[ s5 ];
	b3 = fm_sbuf[ s3 ];
	if( !paired( stp, b5, b3 ) )
		return( 0 );

	s3_5plim = s3lim - stp->s_minlen + 1;
	s3_5plim = MAX( s3_5plim, s3 - stp->s_maxlen + 1 );

/*
fprintf( stderr, "mhlx: s3_5plim = %4d\n", s3_5plim );
fprintf( stderr,
	"mhlx.1: s5 = %4d, s3 = %4d, s3lim =    %3d, b5 = %c, b3 = %c\n",
	s5, s3, s3lim, b5, b3 );
*/

	bpcnt = 1;
	hlen = 1;
	mpr = 0;
	for( bpcnt = 1, hlen = 1, s = s3 - 1; s >= s3_5plim; s--, hlen++ ){
		b5 = fm_sbuf[ s5 + hlen ];
		b3 = fm_sbuf[ s3 - hlen ];
/*
fprintf( stderr,
	"mhlx.2: s5 = %4d, s3 = %4d, s3_5plim = %3d, b5 = %c, b3 = %c\n",
	s5 + hlen, s3 - hlen, s3_5plim, b5, b3 );
*/

		if( paired( stp, b5, b3 ) ){
			bpcnt++;
		}else{
			mpr++;
			if( mpr > stp->s_mispair ){
				if( hlen < stp->s_minlen ){

/*
fprintf( stderr, "mhlx.3: #mpr = %4d, > stp->s_mispair (%4d)\n",
	mpr, stp->s_mispair );
*/

					return( 0 );
				}else
					break;
			}
		}
	}

fprintf( stderr, "mhlx.4: hlx!\n" );
for( s = 0; s < hlen; s++ ){ 
	b5 = fm_sbuf[ s5 + s ];
	b3 = fm_sbuf[ s3 - s ];
	fprintf( stderr, " %4d-%4d: %c-%c\n", s5+s, s3-s, b5, b3 );
}

	return( 1 );
}

static	int	paired( stp, b5, b3 )
STREL_T	*stp;
int	b5;
int	b3;
{
	BP_MAT_T	*bpmatp;
	int	b5i, b3i;
	int	rv;
	
	bpmatp = stp->s_pairset->ps_mat;
	b5i = rm_b2bc[ b5 ];
	b3i = rm_b2bc[ b3 ];
	rv = (*bpmatp)[b5i][b3i];
	return( rv );
}
