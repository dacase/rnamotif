#include <stdio.h>
#include <string.h>
#include "rnamot.h"
#include "y.tab.h"

#define	MIN(a,b)	((a)<(b)?(a):(b))
#define	MAX(a,b)	((a)>(b)?(a):(b))
#define	ODD(i)		((i)&0x1)

extern	int	rm_emsg_lineno;
extern	STREL_T	rm_descr[];
extern	int	rm_n_descr;
extern	int	rm_dminlen;
extern	int	rm_dmaxlen;
extern	SITE_T	*rm_sites;
extern	int	rm_b2bc[];

extern	SEARCH_T	**rm_searches;

extern	int	circf;	/* reg. exp. ^ kludge	*/

static	char	fm_emsg[ 256 ];
static	char	*fm_sid;
static	int	fm_dtype;
static	char	*fm_sdef;
static	int	fm_comp;
static	int	fm_slen;
static	char	*fm_sbuf;
static	int	*fm_winbuf;	/* windowsize + 2, 1 before, 1 after	*/
static	int	*fm_window;	/* fm_winbuf[1]				*/
static	int	fm_windowsize;
static	int	fm_szero;
static	char	*fm_chk_seq;

static	int	find_motif();
static	int	find_1_motif();
static	int	find_ss();
static	int	find_wchlx();
static	int	find_pknot();
static	int	find_phlx();
static	int	find_triplex();
static	int	find_4plex();
static	int	find_4plex_inner();
static	int	match_wchlx();
static	int	match_phlx();
static	int	match_triplex();
static	int	match_4plex();
static	int	paired();
static	int	triple();
static	int	quad();
static	void	mark_ss();
static	void	unmark_ss();
static	void	mark_duplex();
static	void	unmark_duplex();
static	int	chk_wchlx0();
static	int	chk_motif();
static	int	chk_wchlx();
static	int	chk_phlx();
static	int	chk_triplex();
static	int	chk_4plex();
static	int	chk_sites();
static	int	chk_1_site();
static	int	chk_seq();

static	void	print_match();
static	void	mk_cstr();
static	void	set_mbuf();

IDENT_T	*find_id();

int	find_motif_driver( n_searches, searches, sites,
	sid, dtype, sdef, comp, slen, sbuf )
int	n_searches;
SEARCH_T	*searches[];
SITE_T	*sites;
char	sid[];
int	dtype;
char	sdef[];
int	comp;
int	slen;
char	sbuf[];
{
	int	i, w_winsize, slev;
	int	szero, l_szero;
	int	sdollar, f_sdollar, l_sdollar;
	IDENT_T	*ip;
	SEARCH_T	*srp;
	int	rv;
	
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

	fm_sid = sid;
	fm_dtype = dtype;
	fm_sdef = sdef;
	fm_comp = comp;
	fm_slen = slen;
	fm_sbuf = sbuf;

	w_winsize = rm_dmaxlen < fm_windowsize ? rm_dmaxlen : fm_windowsize;

	srp = searches[ 0 ];
	l_szero = slen - w_winsize;
	for( rv = 0, fm_szero = 0; fm_szero < l_szero; fm_szero++ ){
		srp->s_zero = fm_szero;
		srp->s_dollar = fm_szero + w_winsize - 1;
		fm_window[ srp->s_zero - 1 - fm_szero ] = UNDEF;
		fm_window[ srp->s_dollar + 1 - fm_szero ] = UNDEF;
		rv |= find_motif( srp );
	}

	l_szero = slen - rm_dminlen;
	srp->s_dollar = slen - 1;
	for( ; fm_szero <= l_szero; fm_szero++ ){
		srp->s_zero = fm_szero;
		rv |= find_motif( srp );
	}
	return( rv );
}

static	int	find_motif( srp )
SEARCH_T	*srp;
{
	STREL_T	*stp;
	SEARCH_T	*n_srp;
	int	sdollar, o_sdollar, f_sdollar, l_sdollar; 
	int	rv, loop;

	rv = 0;
	stp = srp->s_descr;
	if( stp->s_next != NULL ){
		n_srp = rm_searches[ stp->s_next->s_searchno ]; 
		loop = 1;
	}else if( stp->s_outer == NULL ){
		n_srp = NULL;
		loop = 1;
	}else{
		n_srp = NULL;
		loop = 0;
	}
	o_sdollar = srp->s_dollar;
	f_sdollar = MIN( srp->s_dollar, srp->s_zero + stp->s_maxglen - 1 );
	l_sdollar = srp->s_zero + stp->s_minglen - 1;

	if( loop ){
		rv = 0;
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

static	int	find_1_motif( srp )
SEARCH_T	*srp;
{
	STREL_T	*stp;
	int	rv;

	stp = srp->s_descr;

	switch( stp->s_type ){
	case SYM_SS :
		rv = find_ss( srp );
		break;
	case SYM_H5 :
		if( stp->s_proper ){
			rv = find_wchlx( srp );
		}else{
			rv = find_pknot( srp );
		}
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
		rv = 0;
		rm_emsg_lineno = stp->s_lineno;
		sprintf( fm_emsg, "find_motif: illegal symbol %d.",
			stp->s_type );
		errormsg( 1, fm_emsg );
		break;
	}

	return( rv );
}

static	int	find_ss( srp )
SEARCH_T	*srp;
{
	STREL_T	*stp;
	int	s, slen, szero, sdollar;
	STREL_T	*n_stp;
	SEARCH_T	*n_srp;
	int	rv;

	stp = srp->s_descr;
	szero = srp->s_zero;
	sdollar = srp->s_dollar;
	slen = sdollar - szero + 1;
	if( slen < stp->s_minlen || slen > stp->s_maxlen )
		return( 0 );

	if( stp->s_seq != NULL ){
		if( !chk_seq( stp, &fm_sbuf[ szero ], slen ) )
			return( 0 );
/*
		strncpy( fm_chk_seq, &fm_sbuf[ szero ], slen );
		fm_chk_seq[ slen ] = '\0';
		circf = *stp->s_seq == '^';
		if( stp->s_mismatch > 0 ){
			if( !mm_step( fm_chk_seq,
				stp->s_expbuf, stp->s_mismatch ) )
				return( 0 );
		}else if( !step( fm_chk_seq, stp->s_expbuf ) )
			return( 0 );
*/
	}

	mark_ss( stp, szero, slen);
	n_stp = srp->s_forward;
	if( n_stp != NULL ){
		n_srp = rm_searches[ n_stp->s_searchno ];
		rv = find_motif( n_srp );
	}else{
		rv = 1;
		if( !chk_sites( rm_n_descr, rm_descr, rm_sites ) )
			return( 0 );
		print_match( stdout, fm_sid, fm_comp, rm_n_descr, rm_descr );
/*
		if( chk_motif( rm_n_descr, rm_descr, rm_sites ) ){
			rv = 1;
			print_match( stdout, fm_sid, fm_comp,
				rm_n_descr, rm_descr );
		}else
			rv = 0;
*/
	}
	unmark_ss( stp, szero, slen );
	return( rv );
}

static	int	find_wchlx( srp )
SEARCH_T	*srp;
{
	STREL_T	*stp, *stp3;
	int	s, s3lim, slen, szero, sdollar;
	int	h_minl, h_maxl;
	int	i_minl, i_maxl, i_len;
	int	h, h3, hlen;
	STREL_T	*i_stp;
	SEARCH_T	*i_srp;
	int	rv;

	szero = srp->s_zero;
	sdollar = srp->s_dollar;
	slen = sdollar - szero + 1;
	stp = srp->s_descr;
	stp3 = stp->s_mates[ 0 ];

	h_minl = stp->s_minlen;
	h_maxl = stp->s_maxlen;
	i_minl = stp->s_minilen;
	i_maxl = stp->s_maxilen;

	s3lim = sdollar - szero + 1;
	s3lim = ( s3lim - i_minl ) / 2;
	s3lim = MIN( s3lim, h_maxl );
	s3lim = sdollar - s3lim + 1;

	rv = 0;
	if( match_wchlx( stp, stp3, szero, sdollar, s3lim, &h3, &hlen ) ){

		if( !chk_wchlx0( srp, szero, h3 ) )
			return( 0 );

		i_len = h3 - szero - 2 * hlen + 1;
		if( i_len > i_maxl )
			return( 0 );

		mark_duplex( stp, szero, stp3, h3, hlen );

		i_stp = stp->s_inner;
		i_srp = rm_searches[ i_stp->s_searchno ];
		i_srp->s_zero = szero + hlen;
		i_srp->s_dollar = h3 - hlen;

		rv = find_motif( i_srp );
		unmark_duplex( stp, szero, stp3, h3, hlen );
	}
	return( rv );
}

static	int	find_pknot(  srp )
SEARCH_T	*srp;
{
	STREL_T	*stp, *stp1, *stp2, *stp3;
	int	s, slen, szero, sdollar;
	int	s13_lim, s1_dollar;
	int	h1_minl, h1_maxl;
	int	i1_minl, i1_maxl;
	int	h2_minl, h2_maxl;
	int	i2_minl, i2_maxl;
	int	i3_minl, i3_maxl;
	int	h13, h1len;
	int	s2, s2_zero, s20_lim, s23_lim;
	int	h23, h2len;
	int	i1_len, i2_len, i3_len;
	STREL_T	*i1_stp, *i2_stp, *i3_stp;
	SEARCH_T	*i1_srp, *i2_srp, *i3_srp;
	int	rv;

	szero = srp->s_zero;
	sdollar = srp->s_dollar;
	slen = sdollar - szero + 1;
	stp = srp->s_descr;

	stp1 = stp->s_scopes[ 1 ];
	stp2 = stp->s_scopes[ 2 ];
	stp3 = stp->s_scopes[ 3 ];

	s1_dollar = sdollar - stp2->s_minilen - stp3->s_minlen;

	h1_minl = stp->s_minlen;
	h1_maxl = stp->s_maxlen;
	i1_minl = stp->s_minilen;
	i1_maxl = stp->s_maxilen;
	i1_stp = stp->s_inner;
	i1_srp = rm_searches[ i1_stp->s_searchno ];

	h2_minl = stp1->s_minlen;
	h2_maxl = stp1->s_maxlen;
	i2_minl = stp1->s_minilen;
	i2_maxl = stp1->s_maxilen;
	i2_stp = stp1->s_inner;
	i2_srp = rm_searches[ i2_stp->s_searchno ];

	i3_minl = stp2->s_minilen;
	i3_maxl = stp2->s_maxilen;
	i3_stp = stp2->s_inner;
	i3_srp = rm_searches[ i3_stp->s_searchno ];

	s13_lim = s1_dollar - szero + 1;
	s13_lim = ( s13_lim - i1_minl - h2_minl - i2_minl ) / 2;
	s13_lim = MIN( s13_lim, h1_maxl );
	s13_lim = s1_dollar - s13_lim + 1;

	rv = 0;
	if( match_wchlx( stp, stp2, szero, s1_dollar, s13_lim, &h13, &h1len ) ){

		mark_duplex( stp, szero, stp2, h13, h1len );

		s2_zero = szero + h1len + stp->s_minilen;
		s20_lim = h13 - h1len - stp1->s_minilen - stp1->s_minlen + 1;
		s23_lim = s1_dollar + stp2->s_minilen + 1;

		for( s2 = s2_zero; s2 <= s20_lim; s2++ ){
			if(match_wchlx(stp1,stp3,
				s2,sdollar,s23_lim,&h23,&h2len)){

				i1_len = s2 - szero - h1len;
				if( i1_len > i1_maxl )
					continue;
				i2_len = h13 - h1len - ( s2 + h2len ) + 1;
				if( i2_len > i2_maxl )
					continue;
				i3_len = h23 - h13 - h2len;
				if( i3_len > i3_maxl )
					continue;

				mark_duplex( stp1, s2, stp3, h23, h2len );

				i1_srp->s_zero = szero + h1len;
				i1_srp->s_dollar = s2 - 1;
				i2_srp->s_zero = s2 + h2len;
				i2_srp->s_dollar = h13 - h1len;
				i3_srp->s_zero = h13 + 1;
				i3_srp->s_dollar = h23 - h2len;

				rv |= find_motif( i1_srp );
				unmark_duplex( stp1, s2, stp3, h23, h2len );
			}
		}
		unmark_duplex( stp, szero, stp2, h13, h1len );
	}
	return( rv );
}

static	int	find_phlx( srp )
SEARCH_T	*srp;
{
	STREL_T	*stp, *stp3, *i_stp;
	int	ilen, slen, szero, sdollar;
	int	s, s3lim, s5hi, s5lo;
	int	h_minl, h_maxl;
	int	i_minl, i_maxl, i_len;
	int	h, hlen;
	SEARCH_T	*i_srp;
	int	rv;

	szero = srp->s_zero;
	sdollar = srp->s_dollar;
	slen = sdollar - szero + 1;
	stp = srp->s_descr;
	stp3 = stp->s_mates[ 0 ];

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

	rv = 0;
	if( match_phlx( stp, stp3, szero, sdollar, s5hi, s5lo, &hlen ) ){

		i_len = sdollar - szero - 2 * hlen + 1;
		if( i_len > i_maxl  )
			return( 0 );

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

static	int	find_triplex( srp )
SEARCH_T	*srp;
{
	STREL_T	*stp, *stp1, *stp2;
	STREL_T	*i1_stp, *i2_stp;
	int	slen, szero, sdollar;
	int	s, s1, s3lim, s5hi, s5lo;
	int	h_minl, h_maxl;
	int	i_len, i_minl, i_maxl;
	int	i1_len, i2_len;
	int	i1_minl, i1_maxl, i2_minl, i2_maxl;
	int	h, hlen;
	SEARCH_T	*i1_srp, *i2_srp;
	int	rv;

	szero = srp->s_zero;
	sdollar = srp->s_dollar;
	slen = sdollar - szero + 1;
	stp = srp->s_descr;
	stp1 = stp->s_scopes[ 1 ];
	stp2 = stp->s_scopes[ 2 ];

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

	rv = 0;
	if( match_phlx( stp, stp2, szero, sdollar, s5hi, s5lo, &hlen ) ){

		i_len = sdollar - szero - 2 * hlen + 1;
		if( i_len > i1_maxl + i2_maxl + hlen )
			return( 0 );

		mark_duplex( stp, szero, stp2, sdollar, hlen );

		for( s=sdollar-i2_minl-hlen; s>=szero+2*hlen+i1_minl-1; s-- ){
			if(match_triplex( stp, stp1, szero, s, sdollar, hlen )){

				i1_len = s - 2 * hlen - szero + 1;
				if( i1_len > i1_maxl )
					continue;
				i2_len = sdollar - hlen - s;
				if( i2_len > i2_maxl )
					continue;

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

static	int	find_4plex( srp )
SEARCH_T	*srp;
{
	STREL_T	*stp, *stp1, *stp2, *stp3;
	int	s3lim, slen, szero, sdollar;
	int	s1, s1lim, s2, s2lim;
	int	h_minl, h_maxl;
	int	i_minl, i_maxl, i_len;
	int	i1_minl, i2_minl, i3_minl;
	int	i1_maxl, i2_maxl, i3_maxl;
	int	h, h3, hlen;
	int	rv;

	szero = srp->s_zero;
	sdollar = srp->s_dollar;
	slen = sdollar - szero + 1;
	stp = srp->s_descr;
	stp1 = stp->s_mates[ 0 ];
	stp2 = stp->s_mates[ 1 ];
	stp3 = stp->s_mates[ 2 ];

	h_minl = stp->s_minlen;
	h_maxl = stp->s_maxlen;
	i1_minl = stp->s_minilen;
	i1_maxl = stp->s_maxilen;
	i2_minl = stp1->s_minilen;
	i2_maxl = stp1->s_maxilen;
	i3_minl = stp2->s_minilen;
	i3_maxl = stp2->s_maxilen;

	i_minl = i1_minl + i2_minl + i3_minl + 2 * h_minl;

	s3lim = sdollar - szero + 1;
	s3lim = ( s3lim - i_minl ) / 2;
	s3lim = MIN( s3lim, h_maxl );
	s3lim = sdollar - s3lim + 1;

	rv = 0;
	if( match_wchlx( stp, stp3, szero, sdollar, s3lim, &h3, &hlen ) ){
		mark_duplex( stp, szero, stp3, h3, hlen );
		rv = find_4plex_inner( srp, h3, hlen );
		unmark_duplex( stp, szero, stp3, h3, hlen );
	}
	return( rv );
}

static	int	find_4plex_inner( srp, s3, hlen )
SEARCH_T	*srp;
int	s3;
int	hlen;
{
	STREL_T	*stp, *stp1, *stp2, *stp3;
	int	slen, szero, sdollar;
	int	s, s1, s1lim, s2, s2lim;
	int	h_minl, h_maxl;
	int	i_minl, i_maxl, i_len;
	int	i1_minl, i2_minl, i3_minl;
	int	i1_maxl, i2_maxl, i3_maxl;
	int	i1_len, i2_len, i3_len;
	STREL_T	*i1_stp, *i2_stp, *i3_stp;
	SEARCH_T	*i1_srp, *i2_srp, *i3_srp;
	int	rv;

	szero = srp->s_zero;
	sdollar = srp->s_dollar;
	slen = sdollar - szero + 1;
	stp = srp->s_descr;
	stp1 = stp->s_mates[ 0 ];
	stp2 = stp->s_mates[ 1 ];
	stp3 = stp->s_mates[ 2 ];

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

	i3_minl = stp2->s_minilen;
	i3_maxl = stp2->s_maxilen;
	i3_stp = stp2->s_inner;
	i3_srp = rm_searches[ i3_stp->s_searchno ]; 

	rv = 0;
	s1lim = s3 - 3 * hlen - i3_minl - i2_minl;
	for( s1 = szero + hlen + i1_minl; s1 <= s1lim; s1++ ){
		s2lim = s1 + 2 * hlen + i2_minl;
		for( s2 = s3 - hlen - i3_minl; s2 >= s2lim; s2-- ){
			if( match_4plex( stp1, stp2,
				szero, s1, s2, s3, hlen ) ){

				i1_len = s1 - szero - hlen + 1;
				if( i1_len > i1_maxl )
					continue;
				i2_len = s2 - s1 - 2 * hlen + 1;
				if( i2_len > i2_maxl )
					continue;
				i3_len = s3 - s2 - hlen + 1;
				if( i3_len > i3_maxl )
					continue;

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

static	int	match_wchlx( stp, stp3, s5, s3, s3lim, h3, hlen )
STREL_T	*stp;
STREL_T	*stp3;
int	s5;
int	s3;
int	s3lim;
int	*h3;
int	*hlen;
{
	int	s, s3_5plim;
	int	b5, b3;
	int	mpr, l_pr;
	int	mpcnt[ 100 ];
	int	i_lpb;

	b5 = fm_sbuf[ s5 ];
	b3 = fm_sbuf[ s3 ];

	if( !paired( stp->s_pairset, b5, b3 ) )
		return( 0 );

	*h3 = s3;
	mpcnt[ 0 ] = 0;
	i_lpb = 0;
	l_pr = 1;
	mpr = 0;
/*
	for( bpcnt = 1, *hlen = 1, s = s3 - 1; s >= s3lim; s--, (*hlen)++ ){

		b5 = fm_sbuf[ s5 + *hlen ];
		b3 = fm_sbuf[ s3 - *hlen ];
		if( paired( stp->s_pairset, b5, b3 ) ){
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
	if( bpcnt < stp->s_minlen || bpcnt > stp->s_maxlen )
		return( 0 );
*/
	for( *hlen = 1, s = s3 - 1; s >= s3lim; s-- ){

		b5 = fm_sbuf[ s5 + *hlen ];
		b3 = fm_sbuf[ s3 - *hlen ];
		if( paired( stp->s_pairset, b5, b3 ) ){
			l_pr = 1;
			i_lpb = *hlen;
			mpcnt[ *hlen ] = mpr;
		}else{
			l_pr = 0;
			mpr++;
			mpcnt[ *hlen ] = mpr;
			if( mpr > stp->s_mispair ){
				if( *hlen < stp->s_minlen )
					return( 0 );
				else
					break;
			}
		}
		( *hlen )++;
	}
	if( *hlen != i_lpb + 1 )	/* must end on a pair! */
		*hlen = i_lpb + 1;
	if( *hlen < stp->s_minlen || *hlen > stp->s_maxlen )
		return( 0 );

	if( stp->s_seq != NULL ){
		if( !chk_seq( stp, &fm_sbuf[ s5 ], *hlen ) )
			return( 0 );
/*
		strncpy( fm_chk_seq,  &fm_sbuf[ s5 ], *hlen );
		fm_chk_seq[ *hlen ] = '\0';
		circf = *stp->s_seq == '^';
		if( stp->s_mismatch > 0 ){
			if( !mm_step( fm_chk_seq,
				stp->s_expbuf, stp->s_mismatch ) )
				return( 0 );
		}else if( !step( fm_chk_seq, stp->s_expbuf ) )
			return( 0 );
*/
	}
	if( stp3->s_seq != NULL ){
		if( !chk_seq( stp3, &fm_sbuf[ s3 ], *hlen ) )
			return( 0 );
/*
		strncpy( fm_chk_seq,  &fm_sbuf[ s3 - *hlen + 1 ], *hlen );
		fm_chk_seq[ *hlen ] = '\0';
		circf = *stp3->s_seq == '^';
		if( stp3->s_mismatch > 0 ){
			if( !mm_step( fm_chk_seq,
				stp3->s_expbuf, stp3->s_mismatch ) )
				return( 0 );
		}else if( !step( fm_chk_seq, stp3->s_expbuf ) )
			return( 0 );
*/
	}

	return( 1 );
}

static	int	match_phlx( stp, stp3, s5, s3, s5hi, s5lo, hlen )
STREL_T	*stp;
STREL_T	*stp3;
int	s5;
int	s3;
int	s5hi;
int	s5lo;
int	*hlen;
{
	int	s, s1;
	int	b50, b5, b3;
	int	mpr, l_pr;

	b50 = fm_sbuf[ s5 ];
	b3 = fm_sbuf[ s3 ];

	for( s = s5hi; s >= s5lo; s-- ){
		b5 = fm_sbuf[ s ];
		if( !paired( stp->s_pairset, b5, b3 ) )
			continue;
		*hlen = 1;
		mpr = 0;
		l_pr = 1;
/*
		for( s1 = s - 1; s1 >= s5; s1--, (*hlen)++ ){
			b5 = fm_sbuf[ s1 ];
			b3 = fm_sbuf[ s3 - bpcnt ];
			if( paired( stp->s_pairset, b5, b3 ) ){
				bpcnt++;
			}else{
				mpr++;
				if( mpr > stp->s_mispair )
					return( 0 );
			}
		}
		if( bpcnt < stp->s_minlen || bpcnt > stp->s_maxlen )
			return( 0 );
*/
		for( s1 = s - 1; s1 >= s5; s1-- ){
			b5 = fm_sbuf[ s1 ];
			b3 = fm_sbuf[ s3 - *hlen ];
			if( paired( stp->s_pairset, b5, b3 ) ){
				l_pr = 1;
				( *hlen )++;
			}else{
				l_pr = 0;
				mpr++;
				if( mpr > stp->s_mispair ){
					return( 0 );
				}
				( *hlen )++;
			}
		}
		if( !l_pr )	/* must end on a pair	*/
			return( 0 );
		if( *hlen < stp->s_minlen || *hlen > stp->s_maxlen )
			return( 0 );

		if( stp->s_seq != NULL ){
			if( !chk_seq( stp, &fm_sbuf[ s5 ], *hlen ) )
				return( 0 );
/*
			strncpy( fm_chk_seq, &fm_sbuf[ s5 ], *hlen );
			fm_chk_seq[ *hlen ] = '\0'; 
			circf = *stp->s_seq == '^';
			if( stp->s_mismatch > 0 ){
				if( !mm_step( fm_chk_seq,
					stp->s_expbuf, stp->s_mismatch ) )
					return( 0 );
			} else if( !step( fm_chk_seq, stp->s_expbuf ) )
				return( 0 );
*/
		}
		if( stp3->s_seq != NULL ){
			if( !chk_seq( stp3, &fm_sbuf[ s3-*hlen+1 ], *hlen ) )
				return( 0 );
/*
			strncpy( fm_chk_seq, &fm_sbuf[ s3-*hlen+1 ], *hlen );
			fm_chk_seq[ *hlen ] = '\0'; 
			circf = *stp3->s_seq == '^';
			if( stp3->s_mismatch > 0 ){
				if( !mm_step( fm_chk_seq,
					stp3->s_expbuf, stp3->s_mismatch ) )
					return( 0 );
			}else if( !step( fm_chk_seq, stp3->s_expbuf ) )
				return( 0 );
*/
		}
		return( 1 );
	}
	return( 0 );
}

static	int	match_triplex( stp, stp1, s1, s2, s3, tlen )
STREL_T	*stp;
STREL_T	*stp1;
int	s1;
int	s2;
int	s3;
int	tlen;
{
	int	t;
	int	b1, b2, b3;
	int	mpr, l_pr;

	b1 = fm_sbuf[ s1 ];
	b2 = fm_sbuf[ s2 ];
	b3 = fm_sbuf[ s3 - tlen + 1 ];

	if( !triple( stp->s_pairset, b1, b2, b3 ) )
		return( 0 );
	else
		l_pr = 1;
	
	for( mpr = 0, t = 1; t < tlen; t++ ){
		b1 = fm_sbuf[ s1 + t ];
		b2 = fm_sbuf[ s2 - t ];
		b3 = fm_sbuf[ s3 - tlen + 1 + t ];
		if( !triple( stp->s_pairset, b1, b2, b3 ) ){
			l_pr = 0;
			mpr++;
			if( mpr > stp->s_mispair )
				return( 0 );
		}else
			l_pr = 1;
	}

	if( !l_pr )
		return( 0 );

	if( stp1->s_seq != NULL ){
		if( !chk_seq( stp1, &fm_sbuf[ s2 - tlen + 1 ], tlen ) )
			return( 0 );
/*
		strncpy( fm_chk_seq, &fm_sbuf[ s2 - tlen + 1 ], tlen );
		fm_chk_seq[ tlen ] = '\0';
		circf = *stp1->s_seq == '^';
		if( stp1->s_mismatch > 0 ){
			if( !mm_step( fm_chk_seq,
				stp1->s_expbuf, stp1->s_mismatch ) )
				return( 0 );
		}else if( !step( fm_chk_seq, stp1->s_expbuf ) )
			return( 0 );
*/
	}

	return( 1 );
}

static	int	match_4plex( stp1, stp2, s1, s2, s3, s4, qlen )
STREL_T	*stp1;
STREL_T	*stp2;
int	s1;
int	s2;
int	s3;
int	s4;
int	qlen;
{
	int	q;
	int	b1, b2, b3, b4;
	int	mpr, l_pr;

	b1 = fm_sbuf[ s1 + qlen - 1 ];
	b2 = fm_sbuf[ s2 ];
	b3 = fm_sbuf[ s3 ];
	b4 = fm_sbuf[ s4 - qlen + 1 ];

	if( !quad( stp1->s_pairset, b1, b2, b3, b4 ) )
		return( 0 );
	else
		l_pr = 1;

	for( mpr = 0, q = 1; q < qlen; q++ ){
		b1 = fm_sbuf[ s1 + qlen - 1 - q ];
		b2 = fm_sbuf[ s2 + q ];
		b3 = fm_sbuf[ s3 - q ];
		b4 = fm_sbuf[ s4 - qlen + 1 + q ];
		if( !quad( stp1->s_pairset, b1, b2, b3, b4 ) ){
			l_pr = 0;
			mpr++;
			if( mpr > stp1->s_mispair )
				return( 0 );
		}else
			l_pr = 1;
	}

	if( !l_pr )
		return( 0 );

	if( stp1->s_seq != NULL ){
		if( !chk_seq( stp1, &fm_sbuf[ s2 ], qlen ) )
			return( 0 );
/*
		strncpy( fm_chk_seq, &fm_sbuf[ s2 ], qlen );
		fm_chk_seq[ qlen ] = '\0';
		circf = *stp1->s_seq == '^';
		if( stp1->s_mismatch > 0 ){
			if( !mm_step( fm_chk_seq,
				stp1->s_expbuf, stp1->s_mismatch ) )
				return( 0 );
		}else if( !step( fm_chk_seq, stp1->s_expbuf ) )
			return( 0 );
*/
	}

	if( stp2->s_seq != NULL ){
		if( !chk_seq( stp2, &fm_sbuf[ s3 - qlen + 1 ], qlen ) )
			return( 0 );
/*
		strncpy( fm_chk_seq, &fm_sbuf[ s3 - qlen + 1 ], qlen );
		fm_chk_seq[ qlen ] = '\0';
		circf = *stp2->s_seq == '^';
		if( stp2->s_mismatch > 0 ){
			if( !mm_step( fm_chk_seq,
				stp2->s_expbuf, stp2->s_mismatch ) )
				return( 0 );
		}else if( !step( fm_chk_seq, stp2->s_expbuf ) )
			return( 0 );
*/
	}

	return( 1 );
}

static	int	paired( ps, b5, b3 )
PAIRSET_T	*ps;
int	b5;
int	b3;
{
	BP_MAT_T	*bpmatp;
	int	b5i, b3i;
	int	rv;
	
/*
	bpmatp = stp->s_pairset->ps_mat[ 0 ];
*/
	bpmatp = ps->ps_mat[ 0 ];
	b5i = rm_b2bc[ b5 ];
	b3i = rm_b2bc[ b3 ];
	rv = (*bpmatp)[b5i][b3i];
	return( rv );
}

static	int	triple( ps, b1, b2, b3 )
PAIRSET_T	*ps;
int	b1;
int	b2;
int	b3;
{
	BT_MAT_T	*btmatp;
	int	b1i, b2i, b3i;
	int	rv;
	
/*
	btmatp = stp->s_pairset->ps_mat[ 1 ];
*/
	btmatp = ps->ps_mat[ 1 ];
	b1i = rm_b2bc[ b1 ];
	b2i = rm_b2bc[ b2 ];
	b3i = rm_b2bc[ b3 ];
	rv = (*btmatp)[b1i][b2i][b3i];
	return( rv );
}

static	int	quad( ps, b1, b2, b3, b4 )
PAIRSET_T	*ps;
int	b1;
int	b2;
int	b3;
int	b4;
{
	BQ_MAT_T	*bqmatp;
	int	b1i, b2i, b3i, b4i;
	int	rv;
	
/*
	bqmatp = stp->s_pairset->ps_mat[ 1 ];
*/
	bqmatp = ps->ps_mat[ 1 ];
	b1i = rm_b2bc[ b1 ];
	b2i = rm_b2bc[ b2 ];
	b3i = rm_b2bc[ b3 ];
	b4i = rm_b2bc[ b4 ];
	rv = (*bqmatp)[b1i][b2i][b3i][b4i];
	return( rv );
}

static	void	mark_ss( stp, s5, slen )
STREL_T	*stp;
int	s5;
int	slen;
{
	int	s;

	stp->s_matchoff = s5;
	stp->s_matchlen = slen;

	for( s = 0; s < slen; s++ )
		fm_window[ s5 + s - fm_szero ] = stp->s_index;
}

static	void	unmark_ss( stp, s5, slen )
STREL_T	*stp;
int	s5;
int	slen;
{
	int	s;

	stp->s_matchoff = UNDEF;
	stp->s_matchlen = 0;

	for( s = 0; s < slen; s++ )
		fm_window[ s5 + s - fm_szero ] = UNDEF;
}

static	void	mark_duplex( stp5, h5, stp3, h3, hlen )
STREL_T	*stp5;
int	h5;
STREL_T	*stp3;
int	h3;
int	hlen;
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

static	void	unmark_duplex( stp5, h5, stp3, h3, hlen )
STREL_T	*stp5;
int	h5;
STREL_T	*stp3;
int	h3;
int	hlen;
{
	int	h;

	stp5->s_matchoff = UNDEF;
	stp5->s_matchlen = 0;
	stp3->s_matchoff = UNDEF;
	stp3->s_matchlen = 0;

	for( h = 0; h < hlen; h++ ){
		fm_window[ h5+h-fm_szero ] = UNDEF;
		fm_window[ h3-h-fm_szero ] = UNDEF;
	}
}

static	int	chk_wchlx0( srp, s5, s3 )
SEARCH_T	*srp;
int	s5;
int	s3;
{
	STREL_T	*stp;
	int	b5, b3;

	if( srp->s_backup != NULL )
		return( 1 );
	if( s5 == 0 )
		return( 1 );
	if( s3 == fm_slen - 1 )
		return( 1 );
	b5 = fm_sbuf[ s5 - 1 ];
	b3 = fm_sbuf[ s3 + 1 ];
	stp = srp->s_descr;
	return( !paired( stp->s_pairset, b5, b3 ) );
}

static	int	chk_motif( n_descr, descr, sites )
int	n_descr;
STREL_T	descr[];
SITE_T	*sites;
{
	int	d;
	STREL_T	*stp;

	for( stp = descr, d = 0; d < n_descr; d++, stp++ ){
		switch( stp->s_type ){

		case SYM_H5 :
			if( !chk_wchlx( stp, n_descr, descr ) )
				return( 0 );
			break;

		case SYM_P5 :
			if( !chk_phlx( stp, n_descr, descr ) )
				return( 0 );
			break;
		case SYM_T1 :
			if( !chk_triplex( stp, n_descr, descr ) )
				return( 0 );
			break;
		case SYM_Q1 :
			if( !chk_4plex( stp, n_descr, descr ) )
				return( 0 );
			break;

		default :	
			break;
		}
	}
	if( chk_sites( n_descr, descr, sites ) )
		return( 1 );
	else
		return( 0 );
}

static	int	chk_wchlx( stp, n_descr,descr )
STREL_T	*stp;
int	n_descr;
STREL_T	descr[];
{
	STREL_T	*stp3, *stpd5, *stpd3;
	int	h5_5, h5_3;
	int	h3_5, h3_3;
	int	h5, h3, b5, b3;
	int	d5, d3;

	h5_5 = stp->s_matchoff;
	h5_3 = h5_5 + stp->s_matchlen - 1;

	stp3 = stp->s_mates[ 0 ];
	h3_5 = stp3->s_matchoff;
	h3_3 = h3_5 + stp3->s_matchlen - 1;

	if( h5_5 > 0 ){
		if( h3_3 < fm_slen - 1 ){
			h5 = h5_5 - 1;
			h3 = h3_3 + 1;
			d5 = fm_window[ h5 - fm_szero ];
			d3 = fm_window[ h3 - fm_szero ];
			stpd5 = &descr[ d5 ];
			stpd3 = &descr[ d3 ];
			if( stpd5->s_type==SYM_SS && stpd3->s_type==SYM_SS ){
				b5 = fm_sbuf[ h5 ];
				b3 = fm_sbuf[ h3 ];
				if( paired( stp->s_pairset, b5, b3 ) )
					return( 0 );
			}
		}
	}

	h5 = h5_3 + 1;
	h3 = h3_5 - 1;
	d5 = fm_window[ h5 - fm_szero ];
	d3 = fm_window[ h3 - fm_szero ];
	stpd5 = &descr[ d5 ];
	stpd3 = &descr[ d3 ];
	if( stpd5->s_type==SYM_SS && stpd3->s_type==SYM_SS ){
		b5 = fm_sbuf[ h5 ];
		b3 = fm_sbuf[ h3 ];
		if( paired( stp->s_pairset, b5, b3 ) )
			return( 0 );
	}

	return( 1 );
}

static	int	chk_phlx( stp, n_descr,descr )
STREL_T	*stp;
int	n_descr;
STREL_T	descr[];
{
	STREL_T	*stp3, *stpd5, *stpd3;
	int	h5_5, h5_3;
	int	h3_5, h3_3;
	int	h5, h3, b5, b3;
	int	d5, d3;

	h5_5 = stp->s_matchoff;
	h5_3 = h5_5 + stp->s_matchlen - 1;

	stp3 = stp->s_mates[ 0 ];
	h3_5 = stp3->s_matchoff;
	h3_3 = h3_5 + stp3->s_matchlen - 1;

	if( h5_5 > 0 ){
		h5 = h5_5 - 1;
		h3 = h3_5 - 1;
		d5 = fm_window[ h5 - fm_szero ];
		d3 = fm_window[ h3 - fm_szero ];
		stpd5 = &descr[ d5 ];
		stpd3 = &descr[ d3 ];
		if( stpd5->s_type==SYM_SS && stpd3->s_type==SYM_SS ){
			b5 = fm_sbuf[ h5 ];
			b3 = fm_sbuf[ h3 ];
			if( paired( stp->s_pairset, b5, b3 ) )
				return( 0 );
		}
	}

	if( h3_3 < fm_slen - 1 ){
		h5 = h5_3 + 1;
		h3 = h3_3 + 1;
		d5 = fm_window[ h5 - fm_szero ];
		d3 = fm_window[ h3 - fm_szero ];
		stpd5 = &descr[ d5 ];
		stpd3 = &descr[ d3 ];
		if( stpd5->s_type==SYM_SS && stpd3->s_type==SYM_SS ){
			b5 = fm_sbuf[ h5 ];
			b3 = fm_sbuf[ h3 ];
			if( paired( stp->s_pairset, b5, b3 ) )
				return( 0 );
		}
	}

	return( 1 );
}

static	int	chk_triplex( stp, n_descr,descr )
STREL_T	*stp;
int	n_descr;
STREL_T	descr[];
{
	STREL_T	*stp1, *stp2;
	int	t1_5, t2_3, t3_5;
	int	t1_3, t2_5, t3_3;
	int	t1, t2, t3;
	int	d1, d2, d3;
	int	b1, b2, b3;
	STREL_T	*stpd1, *stpd2, *stpd3;

	t1_5 = stp->s_matchoff;
	t1_3 = t1_5 + stp->s_matchlen - 1;

	stp1 = stp->s_mates[ 0 ];
	t2_5 = stp1->s_matchoff;
	t2_3 = t2_5 + stp1->s_matchlen - 1;

	stp2 = stp->s_mates[ 1 ];
	t3_5 = stp2->s_matchoff;
	t3_3 = t3_5 + stp2->s_matchlen - 1;

	if( t1_5 > 0 ){
		t1 = t1_5 - 1; 
		t2 = t2_3 + 1;
		t3 = t3_5 - 1;
		d1 = fm_window[ t1 - fm_szero ];
		d2 = fm_window[ t2 - fm_szero ];
		d3 = fm_window[ t3 - fm_szero ];
		stpd1 = &descr[ d1 ];
		stpd2 = &descr[ d2 ];
		stpd3 = &descr[ d3 ];
		if( stpd1->s_type==SYM_SS && stpd2->s_type==SYM_SS && 
				stpd3->s_type==SYM_SS){
			b1 = fm_sbuf[ d1 ];
			b2 = fm_sbuf[ d2 ];
			b3 = fm_sbuf[ d3 ];
			if( triple( stp->s_pairset, b1, b2, b3 ) )
				return( 0 );
		}
	}

	if( t3_3 < fm_slen - 1 ){
		t1 = t1_3 + 1; 
		t2 = t2_5 - 1;
		t3 = t3_3 + 1;
		d1 = fm_window[ t1 - fm_szero ];
		d2 = fm_window[ t2 - fm_szero ];
		d3 = fm_window[ t3 - fm_szero ];
		stpd1 = &descr[ d1 ];
		stpd2 = &descr[ d2 ];
		stpd3 = &descr[ d3 ];
		if( stpd1->s_type==SYM_SS && stpd2->s_type==SYM_SS && 
				stpd3->s_type==SYM_SS){
			b1 = fm_sbuf[ d1 ];
			b2 = fm_sbuf[ d2 ];
			b3 = fm_sbuf[ d3 ];
			if( triple( stp->s_pairset, b1, b2, b3 ) )
				return( 0 );
		}
	}

	return( 1 );
}

static	int	chk_4plex( stp, n_descr,descr )
STREL_T	*stp;
int	n_descr;
STREL_T	descr[];
{
	STREL_T	*stp1, *stp2, *stp3;
	int	q1_5, q2_3, q3_5, q4_3;
	int	q1_3, q2_5, q3_3, q4_5;
	int	q1, q2, q3, q4;
	int	d1, d2, d3, d4;
	int	b1, b2, b3, b4;
	STREL_T	*stpd1, *stpd2, *stpd3, *stpd4;

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

	if( q1_5 > 0 ){
		if( q4_3 < fm_slen - 1 ){
			q1 = q1_5 - 1; 
			q2 = q2_3 + 1;
			q3 = q3_5 - 1;
			q4 = q4_3 + 1;
			d1 = fm_window[ q1 - fm_szero ];
			d2 = fm_window[ q2 - fm_szero ];
			d3 = fm_window[ q3 - fm_szero ];
			d4 = fm_window[ q4 - fm_szero ];
			stpd1 = &descr[ d1 ];
			stpd2 = &descr[ d2 ];
			stpd3 = &descr[ d3 ];
			stpd4 = &descr[ d4 ];
			if( stpd1->s_type==SYM_SS && stpd2->s_type==SYM_SS && 
				stpd3->s_type==SYM_SS&&stpd4->s_type==SYM_SS){
				b1 = fm_sbuf[ d1 ];
				b2 = fm_sbuf[ d2 ];
				b3 = fm_sbuf[ d3 ];
				b4 = fm_sbuf[ d4 ];
				if( quad( stp->s_pairset, b1, b2, b3, b4 ) )
					return( 0 );
			}
		}
	}

	q1 = q1_3 + 1; 
	q2 = q2_5 - 1;
	q3 = q3_3 + 1;
	q4 = q4_5 - 1;
	d1 = fm_window[ q1 - fm_szero ];
	d2 = fm_window[ q2 - fm_szero ];
	d3 = fm_window[ q3 - fm_szero ];
	d4 = fm_window[ q4 - fm_szero ];
	stpd1 = &descr[ d1 ];
	stpd2 = &descr[ d2 ];
	stpd3 = &descr[ d3 ];
	stpd4 = &descr[ d4 ];
	if( stpd1->s_type==SYM_SS && stpd2->s_type==SYM_SS && 
		stpd3->s_type==SYM_SS && stpd3->s_type==SYM_SS ){
		b1 = fm_sbuf[ d1 ];
		b2 = fm_sbuf[ d2 ];
		b3 = fm_sbuf[ d3 ];
		b4 = fm_sbuf[ d4 ];
		if( quad( stp->s_pairset, b1, b2, b3, b4 ) )
			return( 0 );
	}

	return( 1 );
}

static	int	chk_sites( n_descr, descr, sites )
int	n_descr;
STREL_T	descr[];
SITE_T	*sites;
{
	SITE_T	*sip;

	for( sip = sites; sip; sip = sip->s_next ){
		if( !chk_1_site( n_descr, descr, sip ) )
			return( 0 );
	} 
	return( 1 );
}

static	int	chk_1_site( n_descr, descr, sip )
int	n_descr;
STREL_T	descr[];
SITE_T	*sip;
{
	int	p;
	POS_T	*pp;
	ADDR_T	*ap;
	STREL_T	*stp;
	int	s[ 4 ];
	int	b[ 4 ];
	int	rv;

	for( pp = sip->s_pos, p = 0; p < sip->s_n_pos; p++, pp++ ){
		stp = &descr[ pp->p_dindex ];
		ap = &pp->p_addr;
		if( ap->a_l2r ){
			if( ap->a_offset > stp->s_matchlen )
				return( 0 );
			else{
				s[ p ] = stp->s_matchoff + ap->a_offset - 1;
				b[ p ] = fm_sbuf[ s[ p ] ];
			}
		}else if( ap->a_offset >= stp->s_matchlen )
			return( 0 );
		else{
			s[ p ] = stp->s_matchoff+stp->s_matchlen-ap->a_offset-1;
			b[ p ] = fm_sbuf[ s[ p ] ];
		}
	}

	if( sip->s_n_pos == 2 ){
		rv = paired( sip->s_pairset, b[ 0 ], b[ 1 ] );
	}else if( sip->s_n_pos == 3 ){
		rv = triple( sip->s_pairset, b[ 0 ], b[ 1 ], b[ 2 ] );
	}else if( sip->s_n_pos == 4 ){
		rv = quad( sip->s_pairset, b[ 0 ], b[ 1 ], b[ 2 ], b[ 3 ] );
	}
	return( rv );
}

static	void	print_match( fp, sid, comp, n_descr, descr )
FILE	*fp;
char	sid[];
int	comp;
int	n_descr;
STREL_T	descr[];
{
	static	int	first = 1;
	char	name[ 20 ];
	int	d, len, offset;
	STREL_T	*stp;
	char	mbuf[ 256 ], cstr[ 256 ];

	if( first ){
		first = 0;
		fprintf( fp, "#RM descr" );
		for( stp = descr, d = 0; d < n_descr; d++, stp++ ){
			RM_strel_name( stp, name );
			fprintf( fp, " %s", name );
			if( stp->s_tag != NULL ){
				mk_cstr( stp->s_tag, cstr );
				fprintf( fp, "(tag=\"%s\")", cstr );
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

	if( fm_dtype == DT_FASTN )
		fprintf( fp, ">%s %s\n", sid, fm_sdef );
	fprintf( fp, "%-12s %d", sid, comp );
	fprintf( fp, " %7d %4d", offset, len );

	for( d = 0; d < n_descr; d++, stp++ ){
		if( stp->s_matchlen > 0 ){
			fprintf( fp, " %.*s",
				stp->s_matchlen, &fm_sbuf[ stp->s_matchoff ] );
		}else
			fprintf( fp, " ." );
	}
	fprintf( fp, "\n" );
}

static	void	mk_cstr( str, cstr )
char	str[];
char	cstr[];
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
static	void	set_mbuf( off, len, mbuf )
int	off;
int	len;
char	mbuf[];
{

	if( len <= 20 ){
		strncpy( mbuf, &fm_sbuf[ off ], len );
		mbuf[ len ] = '\0';
	}else{
		sprintf( mbuf, "%.*s...(%d)...%.*s",
			3, &fm_sbuf[ off ], len,
			3, &fm_sbuf[ off + len - 3 ] );
	}
}

static	int	chk_seq( stp, seq, slen )
STREL_T	*stp;
char	seq[];
int	slen;
{
	int	i;
	char	*csp, *sp;

/*
	strncpy( fm_chk_seq, seq, slen );
	fm_chk_seq[ slen ] = '\0';
*/
	for( csp = fm_chk_seq, sp = seq, i = 0; i < slen; i++, sp++ )
		*csp++ = ( *sp == 'u' || *sp == 'U' ) ? 't' : *sp;
	*csp = '\0';
	circf = *stp->s_seq == '^';
	if( stp->s_mismatch > 0 ){
		return( mm_step( fm_chk_seq, stp->s_expbuf, stp->s_mismatch ) );
	}else
		return( step( fm_chk_seq, stp->s_expbuf ) );
}
