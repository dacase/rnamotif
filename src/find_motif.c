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
static	int	*fm_winbuf;	/* windowsize + 2, 1 before, 1 after	*/
static	int	*fm_window;	/* fm_winbuf[1]				*/
static	int	fm_windowsize;
static	char	*fm_chk_seq;

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
	int	i, w_winsize, slev;
	int	szero, l_szero;
	int	sdollar, f_sdollar, l_sdollar;
	IDENT_T	*ip;
	SEARCH_T	*srp;
	
	if( fm_winbuf == NULL ){
		ip = find_id( "windowsize" );
		if( ip == NULL )
			errormsg( 1,
				"find_motif_driver: windowsize undefined." );
	
			if( ip->i_val.v_value.v_ival <= 0 )
				errormsg( 1,
					"find_motif_driver: windowsize <= 0." );
		else
			fm_windowsize = ip->i_val.v_value.v_ival;
		fm_winbuf = ( int * )malloc( (fm_windowsize+2) * sizeof(int) );
		if( fm_winbuf == NULL )
			errormsg( 1,
				"find_motif_driver: can't allocate fm_winbuf.");
		fm_window = &fm_winbuf[ 1 ];
		fm_chk_seq = ( char * )malloc((fm_windowsize+1) * sizeof(char));
		if( fm_chk_seq == NULL )
			errormsg( 1,
			"find_motif_driver: can't allocate fm_chk_seq." );
	}

fprintf( stderr, "fmd   : locus = %s, slen = %d\n", locus, slen );

	fm_locus = locus;
	fm_slen = slen;
	fm_sbuf = sbuf;

	w_winsize = rm_dmaxlen < fm_windowsize ? rm_dmaxlen : fm_windowsize;

	srp = searches[ 0 ];
	l_szero = slen - w_winsize;
	for( szero = 0; szero < l_szero; szero++ ){
		f_sdollar = szero + w_winsize - 1;
		l_sdollar = szero + rm_dminlen - 1;
fprintf( stderr, "fmd.1 : szero = %4d, sdollar = %4d:%4d\n",
	szero, f_sdollar, l_sdollar );
		sdollar = szero + w_winsize - 1;
		srp->s_zero = szero;
		srp->s_dollar = sdollar;
		find_motif( srp );
	}

	l_szero = slen - rm_dminlen;
	f_sdollar = slen - 1;
	sdollar = slen - 1;
	for( ; szero <= l_szero; szero++ ){
		l_sdollar = szero + rm_dminlen - 1;
fprintf( stderr, "fmd.2 : szero = %4d, sdollar = %4d:%4d\n",
	szero, f_sdollar, l_sdollar );
		srp->s_zero = szero;
		srp->s_dollar = sdollar;
		find_motif( srp );
	}

	return( 0 );
}

static	int	find_motif( srp )
SEARCH_T	*srp;
{
	STREL_T	*stp;

	stp = srp->s_descr;

fprintf( stderr, "fm    : descr = %2d, str = 0, %4d:%4d, %4d\n",
	stp->s_index, srp->s_zero, srp->s_dollar, fm_slen - 1 );

	switch( stp->s_type ){
	case SYM_SS :
/*
		find_ss( slev, n_searches, searches, szero, sdollar );
*/
		break;
	case SYM_H5 :
		if( stp->s_proper ){
			find_wchlx( srp );
		}else{
/*
			find_pknot(  slev, n_searches, searches,
				szero, sdollar );
*/
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

static	int	find_wchlx( srp )
SEARCH_T	*srp;
{
	STREL_T	*stp;
	int	s, s3lim, slen, szero, sdollar;
	int	h_minl, h_maxl;
	int	i_minl;
	int	h3, hlen;

	szero = srp->s_zero;
	sdollar = srp->s_dollar;
	slen = sdollar - szero + 1;
	stp = srp->s_descr;

fprintf( stderr, "fwchlx: descr = %2d, str = 0, %4d:%4d, %4d\n",
	stp->s_index, szero, sdollar, fm_slen - 1 );

	h_minl = stp->s_minlen;
	h_maxl = stp->s_maxlen;
	i_minl = stp->s_minilen;

	s3lim = ( i_minl + 2 * h_minl );
	s3lim = szero + s3lim - 1;

	for( s = sdollar; s >= s3lim; s-- ){
		if( match_helix( stp, szero, s, s3lim, &h3, &hlen ) ){

fprintf( stderr, "fwchlx: %4d %.*s %4d %.*s\n",
	szero+1, hlen, &fm_sbuf[ szero ],
	h3 - hlen + 2, hlen, &fm_sbuf[ h3 - hlen + 1 ] ); 

fprintf( stderr, "fwchlx: inner = %4d %4d; next = %4d, %4d\n",
	szero + hlen + 1, h3 - hlen + 1, h3 + 1, sdollar );

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
static	int	match_helix( stp, s5, s3, s3lim, h3, hlen )
STREL_T	*stp;
int	s5;
int	s3;
int	s3lim;
int	*h3;
int	*hlen;
{
	STREL_T	*stp3;
	int	s, s3_5plim;
	int	b5, b3;
	int	bpcnt, mpr;

	stp3 = stp->s_scopes[ 1 ];
	b5 = fm_sbuf[ s5 ];
	b3 = fm_sbuf[ s3 ];
	if( !paired( stp, b5, b3 ) )
		return( 0 );

	*h3 = s3;
	s3_5plim = s3lim - stp->s_minlen + 1;
	s3_5plim = MAX( s3_5plim, s3 - stp->s_maxlen + 1 );

	bpcnt = 1;
	*hlen = 1;
	mpr = 0;
	for( bpcnt = 1, *hlen = 1, s = s3 - 1; s >= s3_5plim; s--, (*hlen)++ ){
		b5 = fm_sbuf[ s5 + *hlen ];
		b3 = fm_sbuf[ s3 - *hlen ];
		if( paired( stp, b5, b3 ) ){
			bpcnt++;
		}else{
			mpr++;
			if( mpr > stp->s_mispair ){
				if( *hlen < stp->s_minlen )
					return( 0 );
				else
					break;
			}
		}
	}

	if( stp->s_seq != NULL ){
		strncpy( fm_chk_seq,  &fm_sbuf[ s5 ], *hlen );
		fm_chk_seq[ *hlen ] = '\0';
		if( !step( fm_chk_seq, stp->s_expbuf ) )
			return( 0 );
	}else if( stp3->s_seq != NULL ){
		strncpy( fm_chk_seq,  &fm_sbuf[ s3 - *hlen + 1 ], *hlen );
		fm_chk_seq[ *hlen ] = '\0';
		if( !step( fm_chk_seq, stp3->s_expbuf ) )
			return( 0 );
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
