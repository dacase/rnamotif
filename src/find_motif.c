#include <stdio.h>
#include <string.h>
#include <math.h>

#include "rmdefs.h"
#include "rnamot.h"
#include "y.tab.h"

#define	MIN(a,b)	((a)<(b)?(a):(b))
#define	MAX(a,b)	((a)>(b)?(a):(b))
#define	ODD(i)		((i)&0x1)
#define	EPS		1e-6

extern	int	rm_emsg_lineno;
extern	int	rm_strict_helices;
extern	STREL_T	rm_descr[];
extern	int	rm_n_descr;
extern	STREL_T	*rm_o_stp;	/* search for this first	*/
extern	char	*rm_o_expbuf;
extern	STREL_T	*rm_lctx;
extern	int	rm_lctx_explicit;
extern	STREL_T	*rm_rctx;
extern	int	rm_rctx_explicit;
extern	int	rm_dminlen;
extern	int	rm_dmaxlen;
extern	SITE_T	*rm_sites;
extern	int	rm_b2bc[];

extern	SEARCH_T	**rm_searches;

extern	VALUE_T	*rm_sval;

extern	int	circf;	/* reg. exp. ^ kludge	*/
extern	char	*loc1, *loc2;

static	char	fm_emsg[ 256 ];
static	char	*fm_sid;
static	char	*fm_sdef;
static	int	fm_comp;
static	int	fm_slen;
static	char	*fm_sbuf;
static	int	*fm_winbuf;	/* windowsize + 2, 1 before, 1 after	*/
static	int	*fm_window;	/* fm_winbuf[1]				*/
static	int	fm_windowsize;
static	int	fm_szero;
static	int	fm_opt_pos = -1;	/* current opt. subsearch	*/
static	int	fm_opt_lpos = -1;
static	int	fm_opt_rpos = -1;
static	char	*fm_chk_seq;

IDENT_T	*RM_find_id( char [] );

int	RM_paired( PAIRSET_T *, int, int );
int	RM_triple( PAIRSET_T *, int, int, int );
int	RM_quad( PAIRSET_T *, int, int, int, int );

static	int	find_motif( SEARCH_T * );
static	int	adjust_szero( int * );
static	int	find_1_motif( SEARCH_T * );
static	int	find_ss( SEARCH_T * );
static	int	find_wchlx( SEARCH_T * );
static	int	find_pknot( SEARCH_T * );
static	int	find_pknot5( SEARCH_T * );
static	int	find_pknot3( SEARCH_T *, int );
static	int	find_minlen( int, int );
static	int	find_maxlen( int, int );
static	void	upd_pksearches( STREL_T *, int, int, int );
static	int	find_phlx( SEARCH_T * );
static	int	find_triplex( SEARCH_T * );
static	int	find_4plex( SEARCH_T * );
static	int	find_4plex_inner( SEARCH_T *, int, int );
static	int	match_wchlx( STREL_T *, STREL_T *, int, int, int,
			int [], int [], int [] );
static	int	match_phlx( STREL_T *, STREL_T *,
			int, int, int, int, int *, int * );
static	int	match_triplex( STREL_T *, STREL_T *,
			int, int, int, int, int * );
static	int	match_4plex( STREL_T *, STREL_T *,
			int, int, int, int, int, int * );
static	void	mark_ss( STREL_T *, int, int );
static	void	unmark_ss( STREL_T *, int, int );
static	void	mark_duplex( STREL_T *, int, STREL_T *, int, int );
static	void	unmark_duplex( STREL_T *, int, STREL_T *, int, int );
static	int	chk_wchlx0( SEARCH_T *, int, int );
static	int	chk_motif( int, STREL_T [], SITE_T * );
static	int	chk_wchlx( STREL_T *, int, STREL_T [] );
static	int	chk_phlx( STREL_T *, int, STREL_T [] );
static	int	chk_triplex( STREL_T *, int, STREL_T [] );
static	int	chk_4plex( STREL_T *, int, STREL_T [] );
static	int	set_context( int, STREL_T [] );
static	int	chk_sites( int, STREL_T [], SITE_T * );
static	int	chk_1_site( int, STREL_T *, SITE_T * );
static	int	chk_seq( STREL_T *, char [], int );

static	void	print_match( FILE *, char [], int, int, STREL_T [] );
static	void	mk_cstr( char [], char [] );

int	find_motif_driver( int n_searches, SEARCH_T *searches[],
	SITE_T *sites,
	char sid[], char sdef[], int comp, int slen, char sbuf[] )
{
	int	w_winsize;
	int	l_szero;
	IDENT_T	*ip;
	SEARCH_T	*srp;
	int	rv;
	
	if( fm_winbuf == NULL ){
		ip = RM_find_id( "windowsize" );
		if( ip == NULL )
			RM_errormsg( TRUE,
				"find_motif_driver: windowsize undefined." );
	
			if( ip->i_val.v_value.v_ival <= 0 )
				RM_errormsg( TRUE,
					"find_motif_driver: windowsize <= 0." );
		else
			fm_windowsize = ip->i_val.v_value.v_ival;
		fm_winbuf = ( int * )malloc( (fm_windowsize+2) * sizeof(int) );
		if( fm_winbuf == NULL )
			RM_errormsg( TRUE,
				"find_motif_driver: can't allocate fm_winbuf.");
		fm_window = &fm_winbuf[ 1 ];
		fm_chk_seq = ( char * )malloc((fm_windowsize+1) * sizeof(char));
		if( fm_chk_seq == NULL )
			RM_errormsg( TRUE,
			"find_motif_driver: can't allocate fm_chk_seq." );
	}

	fm_sid = sid;
	fm_sdef = sdef;
	fm_comp = comp;
	fm_slen = slen;
	fm_sbuf = sbuf;

	w_winsize = rm_dmaxlen < fm_windowsize ? rm_dmaxlen : fm_windowsize;

	srp = searches[ 0 ];
	l_szero = slen - w_winsize;
	fm_opt_pos = fm_opt_lpos = fm_opt_rpos = -1;
	for( rv = FALSE, fm_szero = 0; fm_szero < l_szero; fm_szero++ ){
		if( rm_o_stp != NULL ){
			if( !adjust_szero( &fm_szero ) )
				return( rv );
		}
		srp->s_zero = fm_szero;
		srp->s_dollar = MIN( fm_szero + w_winsize - 1, slen - 1 );
		fm_window[ srp->s_zero - 1 - fm_szero ] = UNDEF;
		fm_window[ srp->s_dollar + 1 - fm_szero ] = UNDEF;
		rv |= find_motif( srp );
	}

	l_szero = slen - rm_dminlen;
	srp->s_dollar = slen - 1;
	for( ; fm_szero <= l_szero; fm_szero++ ){
		if( rm_o_stp != NULL ){
			if( !adjust_szero( &fm_szero ) )
				return( rv );
		}
		srp->s_zero = fm_szero;
		rv |= find_motif( srp );
	}
	return( rv );
}

static	int	adjust_szero( int *szero )
{
	int	l_mm, n_mm;

	if( *szero <= fm_opt_rpos ){
		return( 1 );
	}else
		fm_opt_pos++;

	circf = FALSE;
	if( ( l_mm = rm_o_stp->s_mismatch ) == 0 ){
		if( step( &fm_sbuf[ fm_opt_pos ], rm_o_expbuf ) ){
			fm_opt_pos = loc1 - fm_sbuf;
			fm_opt_lpos = fm_opt_pos-rm_o_stp->s_bestpat.b_lmaxlen;
			fm_opt_rpos = fm_opt_pos-rm_o_stp->s_bestpat.b_lminlen;
			if( fm_opt_lpos < *szero )
				fm_opt_lpos = *szero;
			if( fm_opt_rpos < *szero )
				fm_opt_rpos = *szero;
			*szero = fm_opt_lpos;
			return( TRUE );
		}
	}else if( mm_step( &fm_sbuf[ fm_opt_pos ], rm_o_expbuf, l_mm, &n_mm ) ){
		fm_opt_pos = loc1 - fm_sbuf;
		fm_opt_lpos = fm_opt_pos-rm_o_stp->s_bestpat.b_lmaxlen;
		fm_opt_rpos = fm_opt_pos-rm_o_stp->s_bestpat.b_lminlen;
		if( fm_opt_lpos < *szero )
			fm_opt_lpos = *szero;
		if( fm_opt_rpos < *szero )
			fm_opt_rpos = *szero;
		*szero = fm_opt_lpos;
		return( TRUE );
	}
	return( FALSE );
}

static	int	find_motif( SEARCH_T *srp )
{
	STREL_T	*stp;
	SEARCH_T	*n_srp;
	int	sdollar, o_sdollar, f_sdollar, l_sdollar; 
	int	rv, loop;

	rv = FALSE;
	stp = srp->s_descr;

	if( stp->s_next != NULL ){
		n_srp = rm_searches[ stp->s_next->s_searchno ]; 
		loop = TRUE;
	}else if( stp->s_outer == NULL ){
		n_srp = NULL;
		loop = TRUE;
	}else{	/* last element of an inner chain.	*/
		n_srp = NULL;
		loop = FALSE;
	}

	o_sdollar = srp->s_dollar;

	f_sdollar = MIN( srp->s_dollar, srp->s_zero + stp->s_maxglen - 1 );
	l_sdollar = srp->s_zero + stp->s_minglen - 1;

	if( loop ){
		rv = FALSE;
		for( sdollar = f_sdollar; sdollar >= l_sdollar; sdollar-- ){
			srp->s_dollar = sdollar;
			if( n_srp != NULL ){
				n_srp->s_zero = sdollar + 1;
				n_srp->s_dollar = o_sdollar;
			}
			rv |= find_1_motif( srp );
		}
	}else
		rv = find_1_motif( srp );

	srp->s_dollar = o_sdollar;

	return( rv );
}

static	int	find_1_motif( SEARCH_T *srp )
{
	STREL_T	*stp;
	int	rv;

	stp = srp->s_descr;
	switch( stp->s_type ){
	case SYM_SS :
		rv = find_ss( srp );
		break;
	case SYM_H5 :
		if( stp->s_attr[ SA_PROPER ] )
			rv = find_wchlx( srp );
		else
			rv = find_pknot( srp );
		break;
	case SYM_P5 :
		rv = find_phlx( srp );
		break;
	case SYM_T1 :
		rv = find_triplex( srp );
		break;
	case SYM_Q1 :
		rv = find_4plex( srp );
		break;
	case SYM_H3 :
	case SYM_P3 :
	case SYM_T2 :
	case SYM_T3 :
	case SYM_Q2 :
	case SYM_Q3 :
	case SYM_Q4 :
	default :
		rv = FALSE;
		rm_emsg_lineno = stp->s_lineno;
		sprintf( fm_emsg, "find_motif: illegal symbol %d.",
			stp->s_type );
		RM_errormsg( TRUE, fm_emsg );
		break;
	}

	return( rv );
}

static	int	find_ss( SEARCH_T *srp )
{
	STREL_T	*stp;
	int	slen, szero, sdollar;
	STREL_T	*n_stp;
	SEARCH_T	*n_srp;
	int	rv;

	stp = srp->s_descr;
	stp->s_n_mismatches = 0;
	stp->s_n_mispairs = 0;
	szero = srp->s_zero;
	sdollar = srp->s_dollar;
	slen = sdollar - szero + 1;

	if( slen < stp->s_minlen || slen > stp->s_maxlen )
		return( FALSE );

	if( stp->s_seq != NULL ){
		if( !chk_seq( stp, &fm_sbuf[ szero ], slen ) )
			return( FALSE );
	}

	mark_ss( stp, szero, slen );
	n_stp = srp->s_forward;
	if( n_stp != NULL ){
		n_srp = rm_searches[ n_stp->s_searchno ];
		rv = find_motif( n_srp );
	}else{
		rv = 1;
		if( rm_strict_helices && 
			!chk_motif( rm_n_descr, rm_descr, rm_sites ) )
		{
				rv = FALSE;
		}else if( !set_context( rm_n_descr, rm_descr ) ){
			rv = FALSE;
		}else if( !chk_sites( rm_n_descr, rm_descr, rm_sites ) ){
			rv = FALSE;
		}else if( RM_score( fm_comp, fm_slen, fm_sbuf ) )
			print_match( stdout,
				fm_sid, fm_comp, rm_n_descr, rm_descr );
/*
		if( !set_context( rm_n_descr, rm_descr ) ){
			rv = FALSE;
		}else if( !chk_sites( rm_n_descr, rm_descr, rm_sites ) ){
			rv = FALSE;
		}else if( RM_score( fm_comp, fm_slen, fm_sbuf ) )
			print_match( stdout,
				fm_sid, fm_comp, rm_n_descr, rm_descr );
*/
/*
		if( chk_motif( rm_n_descr, rm_descr, rm_sites ) ){
			rv = 1;
			print_match( stdout, fm_sid, fm_comp,
				rm_n_descr, rm_descr );
		}else
			rv = FALSE;
*/
	}
	unmark_ss( stp, szero, slen );
	return( rv );
}

static	int	find_wchlx( SEARCH_T *srp )
{
	STREL_T	*stp, *stp3;
	int	s3lim, szero, sdollar;
	int	h_maxl;
	int	i_minl, i_maxl, i_len;
	int	h3[ 101 ], hlen[ 101 ], n_mpr[ 101 ];
	int	h, n_h3;
	STREL_T	*i_stp;
	SEARCH_T	*i_srp;
	int	rv;

	szero = srp->s_zero;
	sdollar = srp->s_dollar;

	stp = srp->s_descr;
	stp->s_n_mismatches = 0;
	stp->s_n_mispairs = 0;
	stp3 = stp->s_mates[ 0 ];
	stp3->s_n_mismatches = 0;
	stp3->s_n_mispairs = 0;

	h_maxl = stp->s_maxlen;
	i_minl = stp->s_minilen;
	i_maxl = stp->s_maxilen;

	s3lim = sdollar - szero + 1;
	s3lim = ( s3lim - i_minl ) / 2;
	s3lim = MIN( s3lim, h_maxl );
	s3lim = sdollar - s3lim + 1;

	rv = FALSE;

	if(n_h3=match_wchlx(stp,stp3,szero,sdollar,s3lim,h3,hlen,n_mpr )){

		for( h = 0; h < n_h3; h++ ){
/*
			if( !chk_wchlx0( srp, szero, h3[h] ) )
				return( FALSE );
*/

			i_len = h3[h] - szero - 2 * hlen[h] + 1;
/*
			if( i_len > i_maxl )
				return( FALSE );
*/
			if( i_len > i_maxl )
				continue;

			stp->s_n_mispairs = n_mpr[h];
			stp3->s_n_mispairs = n_mpr[h];
			mark_duplex( stp, szero, stp3, h3[h], hlen[h] );

			i_stp = stp->s_inner;
			i_srp = rm_searches[ i_stp->s_searchno ];
			i_srp->s_zero = szero + hlen[h];
			i_srp->s_dollar = h3[h] - hlen[h];

			rv |= find_motif( i_srp );
			unmark_duplex( stp, szero, stp3, h3[h], hlen[h] );
		}
	}
	return( rv );
}

static	int	find_pknot( SEARCH_T *srp )
{
	STREL_T	*stp, *stp1;
	int	szero, sdollar;
	int	s;
	SEARCH_T	*srp1;
	int	rv = FALSE;

	szero = srp->s_zero;
	sdollar = srp->s_dollar;
	stp = srp->s_descr;

	if( stp->s_scope == 0 ){ 
		for( s = 1; s < stp->s_n_scopes; s++ ){
			stp1 = stp->s_scopes[ s ];
			if( stp1->s_type == SYM_H5 ){
				stp1->s_matchoff = UNDEF;
				stp1->s_matchlen = UNDEF;
				srp1 = rm_searches[ stp1->s_searchno ];
				srp1->s_zero = szero;
				srp1->s_dollar = sdollar;
			}
		}
	}

	rv = find_pknot5( srp );

	return( rv );
}

static	int	find_pknot5( SEARCH_T *srp )
{
	STREL_T	*stp0, *stp5, *stpn;
	int	szero, sdollar, slen;
	int	p_minl, p_maxl;
	int	r_minl, r_maxl;
	int	s5, f_s5, l_s5;
	int	rv = FALSE;

	szero = srp->s_zero;
	sdollar = srp->s_dollar;
	slen = sdollar - szero + 1;
	stp5 = srp->s_descr;

	stp0 = stp5->s_scopes[ 0 ];
	stpn = stp5->s_scopes[ stp5->s_n_scopes - 1 ];

	p_minl = find_minlen( stp0->s_index, stp5->s_index - 1 );
	p_maxl = find_maxlen( stp0->s_index, stp5->s_index - 1 );

	r_minl = find_minlen( stp5->s_index, stpn->s_index );
	r_maxl = find_maxlen( stp5->s_index, stpn->s_index );

	if( p_maxl + r_maxl < slen )
		return( FALSE );

	f_s5 = szero + p_minl;
	l_s5 = szero + MIN( p_maxl, slen - r_minl );

	for( rv = FALSE, s5 = f_s5; s5 <= l_s5; s5++ )
		rv |= find_pknot3( srp, s5 );

	return( rv );
}

static	int	find_pknot3( SEARCH_T *srp, int s5 )
{
	STREL_T	*stp5, *stp3, *stpn;
	int	sdollar, slen;
	int	g_minl;
	int	h_minl, h_maxl;
	int	i_minl;
	int	s_minl, s_maxl;
	int	s3, f_s3, l_s3;
	int	s3lim;
	int	h3[ 101 ], hlen[ 101 ], n_mpr[ 101 ];
	int	h, n_h3;
	SEARCH_T	*n_srp;
	int	rv = FALSE;

	sdollar = srp->s_dollar;
	slen = sdollar - s5 + 1;
	stp5 = srp->s_descr;

	stp3 = stp5->s_mates[ 0 ];
	stpn = stp5->s_scopes[ stp5->s_n_scopes - 1 ];

	h_minl = stp5->s_minlen;
	h_maxl = stp5->s_maxlen;

	i_minl = find_minlen( stp5->s_index + 1, stp3->s_index - 1 );

	g_minl = 2*h_minl + i_minl;

	s_minl = find_minlen( stp3->s_index + 1, stpn->s_index );
	s_maxl = find_maxlen( stp3->s_index + 1, stpn->s_index );

	if( g_minl + s_minl > slen )
		return( FALSE );

	f_s3 = sdollar - s_minl;
	l_s3 = sdollar - MIN( slen - g_minl, s_maxl );

	for( rv = FALSE, s3 = f_s3; s3 >= l_s3; s3-- ){
		s3lim = s3 - s5 + 1;
		s3lim = ( s3lim - i_minl ) / 2;
		s3lim = MIN( s3lim, h_maxl );
		s3lim = s3 - s3lim + 1;

		if( n_h3=match_wchlx( stp5,stp3,s5,s3,s3lim,h3,hlen,n_mpr )){
			for( h = 0; h < n_h3; h++ ){
				stp5->s_n_mispairs = n_mpr[h];
				stp3->s_n_mispairs = n_mpr[h];
				mark_duplex( stp5, s5, stp3, h3[h], hlen[h] );
				upd_pksearches( stp5, s5, h3[h], hlen[h] );
				n_srp = rm_searches[ stp5->s_searchno + 1 ];
				rv |= find_motif( n_srp );
				unmark_duplex( stp5, s5, stp3, h3[h], hlen[h] );
			}
		}
	}
	
	return( rv );
}

static	int	find_minlen( int fd, int ld )
{
	STREL_T	*stp;
	int	minl, d;

	stp = &rm_descr[ fd ];
	for( minl = 0, d = fd; d <= ld; d++, stp++ ){
		minl += ( stp->s_matchlen != UNDEF ) ?
			stp->s_matchlen : stp->s_minlen;
	}
	return( minl );
}

static	int	find_maxlen( int fd, int ld )
{
	STREL_T	*stp;
	int	maxl, d;

	stp = &rm_descr[ fd ];
	for( maxl = 0, d = fd; d <= ld; d++, stp++ )
		maxl += ( stp->s_matchlen != UNDEF ) ?
			stp->s_matchlen : stp->s_maxlen;
	return( maxl );
}

static	void	upd_pksearches( STREL_T *stp, int h5, int h3, int hlen )
{
	STREL_T	*stp3, *i_stp;
	SEARCH_T	*i_srp;
	
	if( stp->s_scope > 0 ){
		i_stp = stp->s_scopes[ stp->s_scope - 1 ];
		i_stp = i_stp->s_inner;
		if( i_stp != NULL ){
			i_srp = rm_searches[ i_stp->s_searchno ];
			i_srp->s_dollar = h5 - 1;
		}
	}
	i_stp = stp->s_inner;
	if( i_stp != NULL ){
		i_srp = rm_searches[ i_stp->s_searchno ];
		i_srp->s_zero = h5 + hlen;
	}

	stp3 = stp->s_mates[ 0 ];
	i_stp = stp3->s_scopes[ stp3->s_scope - 1 ];
	i_stp = i_stp->s_inner;
	if( i_stp != NULL ){
		i_srp = rm_searches[ i_stp->s_searchno ];
		i_srp->s_dollar = h3 - hlen;
	}

	if( stp3->s_scope < stp3->s_n_scopes - 1 ){
		i_stp = stp3->s_inner;
		if( i_stp != NULL ){
			i_srp = rm_searches[ i_stp->s_searchno ];
			i_srp->s_zero = h3 + 1;
		}
	}
}

static	int	find_phlx( SEARCH_T *srp )
{
	STREL_T	*stp, *stp3, *i_stp;
	int	ilen, slen, szero, sdollar;
	int	s5hi, s5lo;
	int	h_minl, h_maxl;
	int	i_minl, i_maxl, i_len;
	int	hlen, n_mpr;
	SEARCH_T	*i_srp;
	int	rv;

	szero = srp->s_zero;
	sdollar = srp->s_dollar;
	slen = sdollar - szero + 1;
	stp = srp->s_descr;
	stp->s_n_mismatches = 0;
	stp->s_n_mispairs = 0;
	stp3 = stp->s_mates[ 0 ];
	stp3->s_n_mismatches = 0;
	stp3->s_n_mispairs = 0;
	

	h_minl = stp->s_minlen;
	h_maxl = stp->s_maxlen;
	i_minl = stp->s_minilen;
	i_maxl = stp->s_maxilen;

	s5hi = MIN( ( slen - i_minl ) / 2, h_maxl );
	s5hi = szero + s5hi - 1;

	ilen = slen - 2 * h_minl;
	ilen = MIN( ilen, i_maxl );
	s5lo = slen - ilen;
	if( ODD( s5lo ) )
		s5lo++;
	s5lo = MIN( s5lo / 2, h_maxl );
	s5lo = szero + s5lo - 1;

	rv = FALSE;
	if( match_phlx( stp, stp3, szero, sdollar, s5hi, s5lo, &hlen, &n_mpr )){

		i_len = sdollar - szero - 2 * hlen + 1;
		if( i_len > i_maxl  )
			return( FALSE );

		stp->s_n_mispairs = n_mpr;
		stp3->s_n_mispairs = n_mpr;
		mark_duplex( stp, szero, stp3, sdollar, hlen );

		i_stp = stp->s_inner;
		i_srp = rm_searches[ i_stp->s_searchno ];
		i_srp->s_zero = szero + hlen;
		i_srp->s_dollar = sdollar - hlen;

		rv = find_motif( i_srp );
		unmark_duplex( stp, szero, stp3, sdollar, hlen );
	}
	return( rv );
}

static	int	find_triplex( SEARCH_T *srp )
{
	STREL_T	*stp, *stp1, *stp2;
	STREL_T	*i1_stp, *i2_stp;
	int	slen, szero, sdollar;
	int	s, s5hi, s5lo;
	int	h_minl, h_maxl;
	int	i_len;
	int	i1_len, i2_len;
	int	i1_minl, i1_maxl, i2_minl, i2_maxl;
	int	hlen, n_mpr;
	SEARCH_T	*i1_srp, *i2_srp;
	int	rv;

	szero = srp->s_zero;
	sdollar = srp->s_dollar;
	slen = sdollar - szero + 1;
	stp = srp->s_descr;
	stp->s_n_mismatches = 0; 
	stp->s_n_mispairs = 0; 
	stp1 = stp->s_scopes[ 1 ];
	stp1->s_n_mismatches = 0; 
	stp1->s_n_mispairs = 0; 
	stp2 = stp->s_scopes[ 2 ];
	stp2->s_n_mismatches = 0; 
	stp2->s_n_mispairs = 0; 

	h_minl = stp->s_minlen;
	h_maxl = stp->s_maxlen;
	i1_minl = stp->s_minilen;
	i1_maxl = stp->s_maxilen;
	i1_stp = stp->s_inner;
	i1_srp = rm_searches[ i1_stp->s_searchno ];
	i2_minl = stp1->s_minilen;
	i2_maxl = stp1->s_maxilen;
	i2_stp = stp1->s_inner;
	i2_srp = rm_searches[ i2_stp->s_searchno ];

	s5hi = MIN( ( slen - i1_minl - i2_minl ) / 2, h_maxl );
	s5hi = szero + s5hi - 1;

	i_len = slen - 2 * h_minl;
	i_len = MIN( i_len, i1_maxl + h_minl + i2_maxl );
	s5lo = slen - i_len;
	if( ODD( s5lo ) )
		s5lo++;
	s5lo = MIN( s5lo / 2, h_maxl );
	s5lo = szero + s5lo - 1;

	rv = FALSE;
	if( match_phlx( stp, stp2, szero, sdollar, s5hi, s5lo, &hlen, &n_mpr )){

		i_len = sdollar - szero - 2 * hlen + 1;
		if( i_len > i1_maxl + i2_maxl + hlen )
			return( FALSE );

		mark_duplex( stp, szero, stp2, sdollar, hlen );

		for( s=sdollar-i2_minl-hlen; s>=szero+2*hlen+i1_minl-1; s-- ){
			if(match_triplex( stp, stp1, szero, s,
				sdollar, hlen, &n_mpr )){

				i1_len = s - 2 * hlen - szero + 1;
				if( i1_len > i1_maxl )
					continue;
				i2_len = sdollar - hlen - s;
				if( i2_len > i2_maxl )
					continue;

				stp->s_n_mispairs = n_mpr;
				stp1->s_n_mispairs = n_mpr;
				stp2->s_n_mispairs = n_mpr;
				mark_ss( stp1, s - hlen + 1, hlen );

				i1_srp->s_zero = szero + hlen;
				i1_srp->s_dollar = s - hlen;
				i2_srp->s_zero = s + 1;
				i2_srp->s_dollar = sdollar - hlen;

				rv |= find_motif( i1_srp );
				unmark_ss( stp1, s - hlen + 1, hlen );
			}
		}
		unmark_duplex( stp, szero, stp2, sdollar, hlen);
	}
	return( rv );
}

static	int	find_4plex( SEARCH_T *srp )
{
	STREL_T	*stp, *stp1, *stp2, *stp3;
	int	s3lim, szero, sdollar;
	int	h_minl, h_maxl;
	int	i_minl;
	int	i1_minl, i2_minl, i3_minl;
	int	h3[ 101 ], hlen[ 101 ], n_mpr[ 101 ];
	int	h, n_h3;
	int	rv;

	szero = srp->s_zero;
	sdollar = srp->s_dollar;

	stp = srp->s_descr;
	stp->s_n_mismatches = 0;
	stp->s_n_mispairs = 0;
	stp1 = stp->s_mates[ 0 ];
	stp1->s_n_mismatches = 0;
	stp1->s_n_mispairs = 0;
	stp2 = stp->s_mates[ 1 ];
	stp2->s_n_mismatches = 0;
	stp2->s_n_mispairs = 0;
	stp3 = stp->s_mates[ 2 ];
	stp3->s_n_mismatches = 0;
	stp3->s_n_mispairs = 0;

	h_minl = stp->s_minlen;
	h_maxl = stp->s_maxlen;
	i1_minl = stp->s_minilen;
	i2_minl = stp1->s_minilen;
	i3_minl = stp2->s_minilen;

	i_minl = i1_minl + i2_minl + i3_minl + 2 * h_minl;

	s3lim = sdollar - szero + 1;
	s3lim = ( s3lim - i_minl ) / 2;
	s3lim = MIN( s3lim, h_maxl );
	s3lim = sdollar - s3lim + 1;

	rv = FALSE;
	if(n_h3=match_wchlx(stp,stp3,szero,sdollar,s3lim,h3,hlen,n_mpr)){
		for( h = 0; h < n_h3; h++ ){
			mark_duplex( stp, szero, stp3, h3[h], hlen[h] );
			rv |= find_4plex_inner( srp, h3[h], hlen[h] );
			unmark_duplex( stp, szero, stp3, h3[h], hlen[h] );
		}
	}
	return( rv );
}

static	int	find_4plex_inner( SEARCH_T *srp, int s3, int hlen )
{
	STREL_T	*stp, *stp1, *stp2, *stp3;
	int	szero;
	int	s1, s1lim, s2, s2lim;
	int	i1_minl, i2_minl, i3_minl;
	int	i1_maxl, i2_maxl, i3_maxl;
	int	i1_len, i2_len, i3_len, n_mpr;
	STREL_T	*i1_stp, *i2_stp, *i3_stp;
	SEARCH_T	*i1_srp, *i2_srp, *i3_srp;
	int	rv;

	szero = srp->s_zero;

	stp = srp->s_descr;
	stp1 = stp->s_mates[ 0 ];
	stp2 = stp->s_mates[ 1 ];
	stp3 = stp->s_mates[ 2 ];

	i1_minl = stp->s_minilen;
	i1_maxl = stp->s_maxilen;
	i1_stp = stp->s_inner;
	i1_srp = rm_searches[ i1_stp->s_searchno ];
	
	i2_minl = stp1->s_minilen;
	i2_maxl = stp1->s_maxilen;
	i2_stp = stp1->s_inner;
	i2_srp = rm_searches[ i2_stp->s_searchno ];

	i3_minl = stp2->s_minilen;
	i3_maxl = stp2->s_maxilen;
	i3_stp = stp2->s_inner;
	i3_srp = rm_searches[ i3_stp->s_searchno ]; 

	rv = FALSE;
	s1lim = s3 - 3 * hlen - i3_minl - i2_minl;
	for( s1 = szero + hlen + i1_minl; s1 <= s1lim; s1++ ){
		s2lim = s1 + 2 * hlen + i2_minl;
		for( s2 = s3 - hlen - i3_minl; s2 >= s2lim; s2-- ){
			if( match_4plex( stp1, stp2,
				szero, s1, s2, s3, hlen, &n_mpr ) ){

				i1_len = s1 - szero - hlen + 1;
				if( i1_len > i1_maxl )
					continue;
				i2_len = s2 - s1 - 2 * hlen + 1;
				if( i2_len > i2_maxl )
					continue;
				i3_len = s3 - s2 - hlen + 1;
				if( i3_len > i3_maxl )
					continue;

				stp->s_n_mispairs = n_mpr;
				stp1->s_n_mispairs = n_mpr;
				stp2->s_n_mispairs = n_mpr;
				stp3->s_n_mispairs = n_mpr;
				mark_duplex( stp1, s1, stp2, s2, hlen );

				i1_srp->s_zero = szero + hlen;
				i1_srp->s_dollar = s1 - 1;
				i2_srp->s_zero = s1 + hlen;
				i2_srp->s_dollar = s2 - hlen;
				i3_srp->s_zero = s2 + 1;
				i3_srp->s_dollar = s3 - hlen;

				rv |= find_motif( i1_srp );
				unmark_duplex( stp1, s1, stp2, s2, hlen );
			}
		}
	}
	return( rv );
}

static	int	match_wchlx( STREL_T *stp, STREL_T *stp3,
	int s5, int s3, int s3lim, int h3[], int hlen[], int n_mpr[] )
{
	int	nh, hl, mpr, l_bpr;
	int	b5, b3;
	int	mplim; 
	int	pfrac;

	nh = 0;
	b5 = fm_sbuf[ s5 ];
	b3 = fm_sbuf[ s3 ];
	if( stp->s_minlen == 0 ){
		hl = 0;
		mpr = 0;
		if( stp->s_seq != NULL ){
			if( !chk_seq( stp, &fm_sbuf[ s5 ], hl ) )
				goto REAL_HELIX;
		}
		if( stp3->s_seq != NULL ){
			if( chk_seq(stp3, &fm_sbuf[s3-hl+1], hl ) ){
				h3[ nh ] = s3;
				hlen[ nh ] = hl;
				n_mpr[ nh ] = mpr;
				nh++;
			}
		}else{
			h3[ nh ] = s3;
			hlen[ nh ] = hl;
			n_mpr[ nh ] = mpr;
			nh++;
		}
	}

REAL_HELIX : ;

	if( RM_paired( stp->s_pairset, b5, b3 ) ){
		hl = 1;
		mpr = 0;
		l_bpr = TRUE;
	}else if( !( stp->s_attr[ SA_ENDS ] & SA_5PAIRED ) ){
		hl = 1;
		mpr = 1;
		l_bpr = FALSE;
	}else if( stp->s_minlen == 0 )
		return( 1 );
	else
		return( 0 );

	if( stp->s_mispair > 0 ){
		mplim = stp->s_mispair;
		pfrac = 0;
	}else if( stp->s_pairfrac < 1.0 ){
		mplim = (1.-stp->s_pairfrac) *
			MIN(stp->s_maxlen, fm_windowsize) + 0.5;
		pfrac = 1;
	}else{
		mplim = 0;
		pfrac = 0;
	}

	if( hl >= stp->s_minlen ){
		if( !l_bpr && ( stp->s_attr[ SA_ENDS ] & SA_3PAIRED ) )
			goto SKIP;

		if( pfrac ){
			if( 1.*( hl - mpr )/hl < stp->s_pairfrac - EPS )
				goto SKIP;
		}

		if( stp->s_seq != NULL ){
			if( !chk_seq( stp, &fm_sbuf[ s5 ], hl ) )
				goto SKIP;
		}

		if( stp3->s_seq != NULL ){
			if( chk_seq(stp3, &fm_sbuf[s3-hl+1], hl ) ){
				h3[ nh ] = s3;
				hlen[ nh ] = hl;
				n_mpr[ nh ] = mpr;
				nh++;
			}
		}else{
			h3[ nh ] = s3;
			hlen[ nh ] = hl;
			n_mpr[ nh ] = mpr;
			nh++;
		}
	}
SKIP : ;

	for( ; s3 - hl + 1 >= s3lim; ){

		if( hl >= stp->s_maxlen )
			break;

		b5 = fm_sbuf[ s5 + hl ];
		b3 = fm_sbuf[ s3 - hl ];
		if( RM_paired( stp->s_pairset, b5, b3 ) )
			l_bpr = TRUE;
		else{
			mpr++;
			if( mpr > mplim )
				break;
			l_bpr = FALSE;
		}
		hl++;
		if( hl >= stp->s_minlen ){
			if( !l_bpr && ( stp->s_attr[ SA_ENDS ] & SA_3PAIRED ) )
				continue;

			if( pfrac ){	
				if( ( 1.*hl-mpr )/hl < stp->s_pairfrac-EPS )
					continue;
			}

			if( stp->s_seq != NULL ){
				if( !chk_seq( stp, &fm_sbuf[ s5 ], hl ) )
					continue;
			}

			if( stp3->s_seq != NULL ){
				if( chk_seq(stp3, &fm_sbuf[s3-hl+1], hl ) ){
					h3[ nh ] = s3;
					hlen[ nh ] = hl;
					n_mpr[ nh ] = mpr;
					nh++;
				}
			}else{
				h3[ nh ] = s3;
				hlen[ nh ] = hl;
				n_mpr[ nh ] = mpr;
				nh++;
			}
		}
	}

	return( nh );
}

static	int	match_phlx( STREL_T *stp, STREL_T *stp3,
	int s5, int s3, int s5hi, int s5lo, int *hlen, int *n_mpr )
{
	int	s, s1;
	int	b5, b3;
	int	mplim, l_pr;
	int	pfrac;

	mplim = 0;
	pfrac = 0;
	if( stp->s_mispair > 0 )
		mplim = stp->s_mispair;
	else if( stp->s_pairfrac < 1.0 ){
		mplim = (1.-stp->s_pairfrac) * 
			MIN(stp->s_maxlen, fm_windowsize) + 0.5;
		pfrac = 1;
	}

	b3 = fm_sbuf[ s3 ];
	for( s = s5hi; s >= s5lo; s-- ){
		b5 = fm_sbuf[ s ];
		if( RM_paired( stp->s_pairset, b5, b3 ) ){
			*hlen = 1;
			*n_mpr = 0;
			l_pr = TRUE;
		}else if( !( stp->s_attr[ SA_ENDS ] & SA_5PAIRED ) ){
			*hlen = 1;
			*n_mpr = 1;
			l_pr = FALSE;
		}else
			continue;
		for( s1 = s - 1; s1 >= s5; s1-- ){
			b5 = fm_sbuf[ s1 ];
			b3 = fm_sbuf[ s3 - *hlen ];
			if( RM_paired( stp->s_pairset, b5, b3 ) ){
				l_pr = TRUE;
			}else{
				l_pr = FALSE;
				( *n_mpr )++;
				if( *n_mpr > mplim ){
					return( FALSE );
				}
			}
			( *hlen )++;
		}
		if( !l_pr ){
			if( ( stp->s_attr[ SA_ENDS ] & SA_3PAIRED ) )
				return( FALSE );
		}
		if( *hlen < stp->s_minlen || *hlen > stp->s_maxlen )
			return( FALSE );
		if( pfrac ){
			if( 1.*(*hlen-*n_mpr)/(*hlen) < stp->s_pairfrac-EPS )
				return( FALSE );
		}

		if( stp->s_seq != NULL ){
			if( !chk_seq( stp, &fm_sbuf[ s5 ], *hlen ) )
				return( FALSE );
		}
		if( stp3->s_seq != NULL ){
			if( !chk_seq( stp3, &fm_sbuf[ s3-*hlen+1 ], *hlen ) )
				return( FALSE );
		}
		return( TRUE );
	}
	return( FALSE );
}

static	int	match_triplex( STREL_T *stp, STREL_T *stp1,
	int s1, int s2, int s3, int tlen, int *n_mpr )
{
	int	t;
	int	b1, b2, b3;
	int	mplim, l_pr;

	mplim = 0;
	if( stp->s_mispair > 0 )
		mplim = stp->s_mispair;
	else if( stp->s_pairfrac < 1.0 )
		mplim = (1.-stp->s_pairfrac)*tlen + 0.5;
	
	b1 = fm_sbuf[ s1 ];
	b2 = fm_sbuf[ s2 ];
	b3 = fm_sbuf[ s3 - tlen + 1 ];
	if( RM_triple( stp->s_pairset, b1, b2, b3 ) ){
		*n_mpr = 0;
		l_pr = TRUE;
	}else if( !( stp->s_attr[ SA_ENDS ] & SA_5PAIRED ) ){
		*n_mpr = 1;
		l_pr = FALSE;
	}else
		return( FALSE );
	
	for( t = 1; t < tlen; t++ ){
		b1 = fm_sbuf[ s1 + t ];
		b2 = fm_sbuf[ s2 - t ];
		b3 = fm_sbuf[ s3 - tlen + 1 + t ];
		if( !RM_triple( stp->s_pairset, b1, b2, b3 ) ){
			l_pr = FALSE;
			( *n_mpr )++;
			if( *n_mpr > mplim )
				return( FALSE );
		}else
			l_pr = TRUE;
	}

	if( !l_pr ){
		if( stp->s_attr[ SA_ENDS ] & SA_3PAIRED )
			return( FALSE );
	}

	if( stp1->s_seq != NULL ){
		if( !chk_seq( stp1, &fm_sbuf[ s2 - tlen + 1 ], tlen ) )
			return( FALSE );
	}

	return( TRUE );
}

static	int	match_4plex( STREL_T *stp1, STREL_T *stp2,
	int s1, int s2, int s3, int s4, int qlen, int *n_mpr )
{
	int	q;
	int	b1, b2, b3, b4;
	int	mplim, l_pr;

	mplim = 0;
	if( stp1->s_mispair > 0 )
		mplim = stp1->s_mispair;
	else if( stp1->s_pairfrac < 1.0 )
		mplim = (1.-stp1->s_pairfrac)*qlen + 0.5;

	b1 = fm_sbuf[ s1 + qlen - 1 ];
	b2 = fm_sbuf[ s2 ];
	b3 = fm_sbuf[ s3 ];
	b4 = fm_sbuf[ s4 - qlen + 1 ];
	if( RM_quad( stp1->s_pairset, b1, b2, b3, b4 ) ){
		*n_mpr = 0;
		l_pr = TRUE;
	}else if( !( stp1->s_attr[ SA_ENDS ] & SA_5PAIRED ) ){
		*n_mpr = 1;
		l_pr = FALSE;
	}else
		return( FALSE );

	for( *n_mpr = 0, q = 1; q < qlen; q++ ){
		b1 = fm_sbuf[ s1 + qlen - 1 - q ];
		b2 = fm_sbuf[ s2 + q ];
		b3 = fm_sbuf[ s3 - q ];
		b4 = fm_sbuf[ s4 - qlen + 1 + q ];
		if( !RM_quad( stp1->s_pairset, b1, b2, b3, b4 ) ){
			l_pr = FALSE;
			( *n_mpr )++;
			if( *n_mpr > mplim )
				return( FALSE );
		}else
			l_pr = TRUE;
	}
	if( !l_pr ){
		if( stp1->s_attr[ SA_ENDS ] & SA_3PAIRED )
			return( FALSE );
	}

	if( stp1->s_seq != NULL ){
		if( !chk_seq( stp1, &fm_sbuf[ s2 ], qlen ) )
			return( FALSE );
	}

	if( stp2->s_seq != NULL ){
		if( !chk_seq( stp2, &fm_sbuf[ s3 - qlen + 1 ], qlen ) )
			return( FALSE );
	}

	return( TRUE );
}

int	RM_paired( PAIRSET_T *ps, int b5, int b3 )
{
	BP_MAT_T	*bpmatp;
	int	b5i, b3i;
	int	rv;
	
	bpmatp = ps->ps_mat[ 0 ];
	b5i = rm_b2bc[ b5 ];
	b3i = rm_b2bc[ b3 ];
	rv = (*bpmatp)[b5i][b3i];
	return( rv );
}

int	RM_triple( PAIRSET_T *ps, int b1, int b2, int b3 )
{
	BT_MAT_T	*btmatp;
	int	b1i, b2i, b3i;
	int	rv;
	
	btmatp = ps->ps_mat[ 1 ];
	b1i = rm_b2bc[ b1 ];
	b2i = rm_b2bc[ b2 ];
	b3i = rm_b2bc[ b3 ];
	rv = (*btmatp)[b1i][b2i][b3i];
	return( rv );
}

int	RM_quad( PAIRSET_T *ps, int b1, int b2, int b3, int b4 )
{
	BQ_MAT_T	*bqmatp;
	int	b1i, b2i, b3i, b4i;
	int	rv;
	
	bqmatp = ps->ps_mat[ 1 ];
	b1i = rm_b2bc[ b1 ];
	b2i = rm_b2bc[ b2 ];
	b3i = rm_b2bc[ b3 ];
	b4i = rm_b2bc[ b4 ];
	rv = (*bqmatp)[b1i][b2i][b3i][b4i];
	return( rv );
}

static	void	mark_ss( STREL_T *stp, int s5, int slen )
{
	int	s;

	stp->s_matchoff = s5;
	stp->s_matchlen = slen;

	for( s = 0; s < slen; s++ )
		fm_window[ s5 + s - fm_szero ] = stp->s_index;
}

static	void	unmark_ss( STREL_T *stp, int s5, int slen )
{
	int	s;

	stp->s_matchoff = UNDEF;
	stp->s_matchlen = UNDEF;

	for( s = 0; s < slen; s++ )
		fm_window[ s5 + s - fm_szero ] = UNDEF;
}

static	void	mark_duplex( STREL_T *stp5, int h5,
	STREL_T *stp3, int h3, int hlen )
{
	int	h;

	stp5->s_matchoff = h5;
	stp5->s_matchlen = hlen;
	stp3->s_matchoff = h3 - hlen + 1;
	stp3->s_matchlen = hlen;

	for( h = 0; h < hlen; h++ ){
		fm_window[ h5+h-fm_szero ] = stp5->s_index;
		fm_window[ h3-h-fm_szero ] = stp5->s_index;
	}
}

static	void	unmark_duplex( STREL_T *stp5, int h5,
	STREL_T *stp3, int h3, int hlen )
{
	int	h;

	stp5->s_matchoff = UNDEF;
	stp5->s_matchlen = UNDEF;
	stp3->s_matchoff = UNDEF;
	stp3->s_matchlen = UNDEF;

	for( h = 0; h < hlen; h++ ){
		fm_window[ h5+h-fm_szero ] = UNDEF;
		fm_window[ h3-h-fm_szero ] = UNDEF;
	}
}

static	int	chk_wchlx0( SEARCH_T *srp, int s5, int s3 )
{
	STREL_T	*stp;
	int	b5, b3;

	if( srp->s_backup != NULL )
		return( TRUE );
	if( s5 == 0 )
		return( TRUE );
	if( s3 == fm_slen - 1 )
		return( TRUE );
	b5 = fm_sbuf[ s5 - 1 ];
	b3 = fm_sbuf[ s3 + 1 ];
	stp = srp->s_descr;
	return( !RM_paired( stp->s_pairset, b5, b3 ) );
}

static	int	chk_motif( int n_descr, STREL_T descr[], SITE_T *sites )
{
	int	d;
	STREL_T	*stp;

	for( stp = descr, d = 0; d < n_descr; d++, stp++ ){
		if( !stp->s_attr[ SA_STRICT ] )
			continue;

		switch( stp->s_type ){
		case SYM_H5 :
			if( !chk_wchlx( stp, n_descr, descr ) )
				return( FALSE );
			break;

		case SYM_P5 :
			if( !chk_phlx( stp, n_descr, descr ) )
				return( FALSE );
			break;
		case SYM_T1 :
			if( !chk_triplex( stp, n_descr, descr ) )
				return( FALSE );
			break;
		case SYM_Q1 :
			if( !chk_4plex( stp, n_descr, descr ) )
				return( FALSE );
			break;

		default :	
			break;
		}
	}
	return( TRUE );
}

static	int	chk_wchlx( STREL_T *stp, int n_descr, STREL_T descr[] )
{
	STREL_T	*stp3;
	int	h5_5, h5_3;
	int	h3_5, h3_3;
	int	h5, h3, b5, b3;
	int	d5, d3;
	int	st5, st3;

	h5_5 = stp->s_matchoff;
	h5_3 = h5_5 + stp->s_matchlen - 1;

	stp3 = stp->s_mates[ 0 ];
	h3_5 = stp3->s_matchoff;
	h3_3 = h3_5 + stp3->s_matchlen - 1;

	if( stp->s_attr[ SA_STRICT ] & SA_5STRICT ){
		if( h5_5 > 0 && h3_3 < fm_slen - 1 ){
			h5 = h5_5 - 1;
			if( ( d5 = fm_window[ h5 - fm_szero ] ) == UNDEF )
				st5 = SYM_SS;
			else
				st5 = descr[ d5 ].s_type;
				
			h3 = h3_3 + 1;
			if( ( d3 = fm_window[ h3 - fm_szero ] ) == UNDEF )
				st3 = SYM_SS;
			else
				st3 = descr[ d3 ].s_type;

			if( st5 == SYM_SS && st3 == SYM_SS ){
				b5 = fm_sbuf[ h5 ];
				b3 = fm_sbuf[ h3 ];
				if( RM_paired( stp->s_pairset, b5, b3 ) )
					return( FALSE );
			}
		}
	}

	if( stp->s_attr[ SA_STRICT ] & SA_3STRICT ){
		h5 = h5_3 + 1;
		d5 = fm_window[ h5 - fm_szero ];
		st5 = descr[ d5 ].s_type;

		h3 = h3_5 - 1;
		d3 = fm_window[ h3 - fm_szero ];
		st3 = descr[ d3 ].s_type;

		if( st5 == SYM_SS && st3 == SYM_SS ){
			b5 = fm_sbuf[ h5 ];
			b3 = fm_sbuf[ h3 ];
			if( RM_paired( stp->s_pairset, b5, b3 ) )
				return( FALSE );
		}
	}

	return( TRUE );
}

static	int	chk_phlx( STREL_T *stp, int n_descr, STREL_T descr[] )
{
	STREL_T	*stp3;
	int	h5_5, h5_3;
	int	h3_5, h3_3;
	int	h5, h3, b5, b3;
	int	d5, d3;
	int	st5, st3;

	h5_5 = stp->s_matchoff;
	h5_3 = h5_5 + stp->s_matchlen - 1;

	stp3 = stp->s_mates[ 0 ];
	h3_5 = stp3->s_matchoff;
	h3_3 = h3_5 + stp3->s_matchlen - 1;

	if( ( stp->s_attr[ SA_STRICT ] & SA_5STRICT ) && h5_5 > 0 ){
		h5 = h5_5 - 1;
		if( ( d5 = fm_window[ h5 - fm_szero ] ) == UNDEF )
			st5 = SYM_SS;
		else
			st5 = descr[ d5 ].s_type;

		h3 = h3_5 - 1;
		d3 = fm_window[ h3 - fm_szero ];
		st3 = descr[ d3 ].s_type;

		if( st5 == SYM_SS && st3 == SYM_SS ){
			b5 = fm_sbuf[ h5 ];
			b3 = fm_sbuf[ h3 ];
			if( RM_paired( stp->s_pairset, b5, b3 ) )
				return( TRUE );
		}
	}

	if( ( stp->s_attr[ SA_STRICT ] & SA_3STRICT ) && h3_3 < fm_slen - 1 ){
		h5 = h5_3 + 1;
		d5 = fm_window[ h5 - fm_szero ];
		st5 = descr[ d5 ].s_type;

		h3 = h3_3 + 1;
		if( ( d3 = fm_window[ h3 - fm_szero ] ) == UNDEF )
			st3 = SYM_SS;
		else
			st3 = descr[ d3 ].s_type;

		if( st5 == SYM_SS && st3 == SYM_SS ){
			b5 = fm_sbuf[ h5 ];
			b3 = fm_sbuf[ h3 ];
			if( RM_paired( stp->s_pairset, b5, b3 ) )
				return( TRUE );
		}
	}

	return( TRUE );
}

static	int	chk_triplex( STREL_T *stp, int n_descr, STREL_T descr[] )
{
	STREL_T	*stp1, *stp2;
	int	t1_5, t2_3, t3_5;
	int	t1_3, t2_5, t3_3;
	int	t1, t2, t3;
	int	d1, d2, d3;
	int	b1, b2, b3;
	int	st1, st2, st3;

	t1_5 = stp->s_matchoff;
	t1_3 = t1_5 + stp->s_matchlen - 1;

	stp1 = stp->s_mates[ 0 ];
	t2_5 = stp1->s_matchoff;
	t2_3 = t2_5 + stp1->s_matchlen - 1;

	stp2 = stp->s_mates[ 1 ];
	t3_5 = stp2->s_matchoff;
	t3_3 = t3_5 + stp2->s_matchlen - 1;

	if( ( stp->s_attr[ SA_STRICT ] & SA_5STRICT ) && t1_5 > 0 ){
		t1 = t1_5 - 1; 
		if( ( d1 = fm_window[ t1 - fm_szero ] ) == UNDEF )
			st1 = SYM_SS;
		else
			st1 = descr[ d1 ].s_type;

		t2 = t2_3 + 1;
		d2 = fm_window[ t2 - fm_szero ];
		st2 = descr[ d2 ].s_type;

		t3 = t3_5 - 1;
		d3 = fm_window[ t3 - fm_szero ];
		st3 = descr[ d3 ].s_type;

		if( st1 == SYM_SS && st2 == SYM_SS && st3 == SYM_SS ){
			b1 = fm_sbuf[ t1 ];
			b2 = fm_sbuf[ t2 ];
			b3 = fm_sbuf[ t3 ];
			if( RM_triple( stp->s_pairset, b1, b2, b3 ) )
				return( FALSE );
		}
	}

	if( ( stp->s_attr[ SA_STRICT ] & SA_3STRICT ) && t3_3 < fm_slen - 1 ){
		t1 = t1_3 + 1; 
		d1 = fm_window[ t1 - fm_szero ];
		st1 = descr[ d1 ].s_type;

		t2 = t2_5 - 1;
		d2 = fm_window[ t2 - fm_szero ];
		st2 = descr[ d2 ].s_type;

		t3 = t3_3 + 1;
		if( ( d3 = fm_window[ t3 - fm_szero ] ) == UNDEF )
			st3 = SYM_SS;
		else
			st3 = descr[ d3 ].s_type;

		if( st1 == SYM_SS && st2 == SYM_SS && st3 == SYM_SS ){
			b1 = fm_sbuf[ t1 ];
			b2 = fm_sbuf[ t2 ];
			b3 = fm_sbuf[ t3 ];
			if( RM_triple( stp->s_pairset, b1, b2, b3 ) )
				return( FALSE );
		}
	}

	return( TRUE );
}

static	int	chk_4plex( STREL_T *stp, int n_descr, STREL_T descr[] )
{
	STREL_T	*stp1, *stp2, *stp3;
	int	q1_5, q2_3, q3_5, q4_3;
	int	q1_3, q2_5, q3_3, q4_5;
	int	q1, q2, q3, q4;
	int	d1, d2, d3, d4;
	int	b1, b2, b3, b4;
	int	st1, st2, st3, st4;

	q1_5 = stp->s_matchoff;
	q1_3 = q1_5 + stp->s_matchlen - 1;

	stp1 = stp->s_mates[ 0 ];
	q2_5 = stp1->s_matchoff;
	q2_3 = q2_5 + stp1->s_matchlen - 1;

	stp2 = stp->s_mates[ 1 ];
	q3_5 = stp2->s_matchoff;
	q3_3 = q3_5 + stp2->s_matchlen - 1;

	stp3 = stp->s_mates[ 2 ];
	q4_5 = stp3->s_matchoff;
	q4_3 = q4_5 + stp3->s_matchlen - 1;

	if( stp->s_attr[ SA_STRICT ] & SA_5STRICT ){
		if( q1_5 > 0 && q4_3 < fm_slen - 1 ){
			q1 = q1_5 - 1; 
			if( ( d1 = fm_window[ q1 - fm_szero ] ) == UNDEF )
				st1 = SYM_SS;
			else
				st1 = descr[ d1 ].s_type;

			q2 = q2_3 + 1;
			d2 = fm_window[ q2 - fm_szero ];
			st2 = descr[ d2 ].s_type;

			q3 = q3_5 - 1;
			d3 = fm_window[ q3 - fm_szero ];
			st3 = descr[ d3 ].s_type;

			q4 = q4_3 + 1;
			if( ( d4 = fm_window[ q4 - fm_szero ] ) == UNDEF )
				st4 = SYM_SS;
			else
				st4 = descr[ d4 ].s_type;

			if( st1 == SYM_SS && st2 == SYM_SS && 
				st3 ==SYM_SS && st4 == SYM_SS )
			{
				b1 = fm_sbuf[ q1 ];
				b2 = fm_sbuf[ q2 ];
				b3 = fm_sbuf[ q3 ];
				b4 = fm_sbuf[ q4 ];
				if( RM_quad( stp->s_pairset, b1, b2, b3, b4 ) )
					return( FALSE );
			}
		}
	}

	if( stp->s_attr[ SA_STRICT ] & SA_3STRICT ){
		q1 = q1_3 + 1; 
		d1 = fm_window[ q1 - fm_szero ];
		st1 = descr[ d1 ].s_type;

		q2 = q2_5 - 1;
		d2 = fm_window[ q2 - fm_szero ];
		st2 = descr[ d2 ].s_type;

		q3 = q3_3 + 1;
		d3 = fm_window[ q3 - fm_szero ];
		st3 = descr[ d3 ].s_type;

		q4 = q4_5 - 1;
		d4 = fm_window[ q4 - fm_szero ];
		st4 = descr[ d4 ].s_type;

		if( st1 == SYM_SS && st2 == SYM_SS && 
			st3 == SYM_SS && st3 == SYM_SS ){
			b1 = fm_sbuf[ q1 ];
			b2 = fm_sbuf[ q2 ];
			b3 = fm_sbuf[ q3 ];
			b4 = fm_sbuf[ q4 ];
			if( RM_quad( stp->s_pairset, b1, b2, b3, b4 ) )
				return( FALSE );
		}
	}

	return( TRUE );
}

static	int	set_context( int n_descr, STREL_T descr[] )
{
	STREL_T	*stp;
	int	offset, length;
	int	rv = TRUE;

	if( rm_lctx == NULL ){
		if( rm_rctx == NULL )
			return( TRUE );
	}else{
		stp = &descr[ 0 ];
		offset = rm_lctx->s_matchoff =
			MAX( stp->s_matchoff - rm_lctx->s_maxlen, 0 );
		length = rm_lctx->s_matchlen = 
			stp->s_matchoff - rm_lctx->s_matchoff;
		if( length < rm_lctx->s_minlen )
			return( FALSE );
		if( rm_lctx->s_seq != NULL ){
			if( !chk_seq( rm_lctx, &fm_sbuf[ offset ], length ) )
				return( FALSE );
		}
	}
	if( rm_rctx != NULL ){
		stp = &descr[ n_descr - 1 ];
		rm_rctx->s_matchoff = stp->s_matchoff + stp->s_matchlen;
		offset = MIN( rm_rctx->s_matchoff+rm_rctx->s_maxlen, fm_slen );
		length = rm_rctx->s_matchlen = offset - rm_rctx->s_matchoff;
		if( length < rm_rctx->s_minlen )
			return( FALSE );
		if( rm_rctx->s_seq != NULL ){
			if( !chk_seq( rm_rctx, &fm_sbuf[ offset ], length ) )
				return( FALSE );
		}
	}

	return( rv );
}

static	int	chk_sites( int n_descr, STREL_T descr[], SITE_T *sites )
{
	SITE_T	*sip;

	for( sip = sites; sip; sip = sip->s_next ){
		if( !chk_1_site( n_descr, descr, sip ) )
			return( FALSE );
	} 
	return( TRUE );
}

static	int	chk_1_site( int n_descr, STREL_T descr[], SITE_T *sip )
{
	int	p;
	POS_T	*pp;
	ADDR_T	*ap;
	STREL_T	*stp;
	int	s[ 4 ];
	int	b[ 4 ];
	int	rv;

	for( pp = sip->s_pos, p = 0; p < sip->s_n_pos; p++, pp++ ){
/*
		stp = &descr[ pp->p_dindex ];
*/
		stp = pp->p_descr;
		ap = &pp->p_addr;
		if( ap->a_l2r ){
			if( ap->a_offset > stp->s_matchlen )
				return( FALSE );
			else{
				s[ p ] = stp->s_matchoff + ap->a_offset - 1;
				b[ p ] = fm_sbuf[ s[ p ] ];
			}
		}else if( ap->a_offset >= stp->s_matchlen )
			return( FALSE );
		else{
			s[ p ] = stp->s_matchoff+stp->s_matchlen-ap->a_offset-1;
			b[ p ] = fm_sbuf[ s[ p ] ];
		}
	}

	if( sip->s_n_pos == 2 ){
		rv = RM_paired( sip->s_pairset, b[ 0 ], b[ 1 ] );
	}else if( sip->s_n_pos == 3 ){
		rv = RM_triple( sip->s_pairset, b[ 0 ], b[ 1 ], b[ 2 ] );
	}else if( sip->s_n_pos == 4 ){
		rv = RM_quad( sip->s_pairset, b[ 0 ], b[ 1 ], b[ 2 ], b[ 3 ] );
	}
	return( rv );
}

static	int	chk_seq( STREL_T *stp, char seq[], int slen )
{
	int	i;
	char	*csp, *sp;

	for( csp = fm_chk_seq, sp = seq, i = 0; i < slen; i++, sp++ )
		*csp++ = ( *sp == 'u' || *sp == 'U' ) ? 't' : *sp;
	*csp = '\0';
	circf = *stp->s_seq == '^';
	if( stp->s_mismatch > 0 ){
		return( mm_step( fm_chk_seq, stp->s_expbuf, stp->s_mismatch,
			&stp->s_n_mismatches ) );
	}else
		return( step( fm_chk_seq, stp->s_expbuf ) );
}

static	void	print_match( FILE *fp, char sid[], int comp,
	int n_descr, STREL_T descr[] )
{
	static	int	first = 1;
	char	name[ 20 ];
	int	d, len, offset;
	STREL_T	*stp;
	char	cstr[ 256 ];

	if( first ){
		first = 0;
		fprintf( fp, "#RM scored\n" );
		fprintf( fp, "#RM descr" );
		if( rm_lctx != NULL ){
			RM_strel_name( rm_lctx, name );
			fprintf( fp, " %s", name );
			if( rm_lctx->s_tag != NULL ){
				mk_cstr( rm_lctx->s_tag, cstr );
				fprintf( fp, "(tag='%s')", cstr );
			}
		}
		for( stp = descr, d = 0; d < n_descr; d++, stp++ ){
			RM_strel_name( stp, name );
			fprintf( fp, " %s", name );
			if( stp->s_tag != NULL ){
				mk_cstr( stp->s_tag, cstr );
				fprintf( fp, "(tag='%s')", cstr );
			}
		}
		if( rm_rctx != NULL ){
			RM_strel_name( rm_rctx, name );
			fprintf( fp, " %s", name );
			if( rm_rctx->s_tag != NULL ){
				mk_cstr( rm_rctx->s_tag, cstr );
				fprintf( fp, "(tag='%s')", cstr );
			}
		}
		fprintf( fp, "\n" );
	}
	for( stp = descr, len = 0, d = 0; d < n_descr; d++, stp++ )
		len += stp->s_matchlen;

	stp = descr; 
	if( comp ){
		offset = fm_slen - stp->s_matchoff;
	}else
		offset = stp->s_matchoff + 1;

	fprintf( fp, ">%s %s\n", sid, fm_sdef );
	fprintf( fp, "%-12s", sid );
	switch( rm_sval->v_type ){
	case T_INT :
		fprintf( fp, " %8d", rm_sval->v_value.v_ival );
		break;
	case T_FLOAT :
		fprintf( fp, " %8.3lf", rm_sval->v_value.v_dval );
		break;
	case T_STRING :
		fprintf( fp, " %8s", rm_sval->v_value.v_pval );
		break;
	default :
		fprintf( fp, " %8.3lf", 0.0 );
		break;
	}
	fprintf( fp, " %d %7d %4d", comp, offset, len );

	if( rm_lctx != NULL ){
		if( rm_lctx->s_matchlen > 0 ){
			fprintf( fp, " %.*s",
				rm_lctx->s_matchlen,
				&fm_sbuf[ rm_lctx->s_matchoff ] );
		}else
			fprintf( fp, " ." );
	}
	for( d = 0; d < n_descr; d++, stp++ ){
		if( stp->s_matchlen > 0 ){
			fprintf( fp, " %.*s",
				stp->s_matchlen, &fm_sbuf[ stp->s_matchoff ] );
		}else
			fprintf( fp, " ." );
	}
	if( rm_rctx != NULL ){
		if( rm_rctx->s_matchlen > 0 ){
			fprintf( fp, " %.*s",
				rm_rctx->s_matchlen,
				&fm_sbuf[ rm_rctx->s_matchoff ] );
		}else
			fprintf( fp, " ." );
	}
	fprintf( fp, "\n" );
}

static	void	mk_cstr( char str[], char cstr[] )
{
	char	*sp, *cp;

	if( str == NULL ){
		*cstr = '\0';
		return;
	}

	for( sp = str, cp = cstr; *sp; sp++ ){
		if( *sp == '"' || *sp == '\\' )
			*cp++ = '\\';
		*cp++ = *sp;
	}
	*cp = '\0';
}
