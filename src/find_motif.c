#include <stdio.h>
#include <string.h>
#include "rnamot.h"
#include "y.tab.h"

#define	MIN(a,b)	((a)<(b)?(a):(b))
#define	MAX(a,b)	((a)>(b)?(a):(b))

extern	int	rm_emsg_lineno;
extern	STREL_T	rm_descr[];
extern	int	rm_n_descr;
extern	int	rm_dminlen;
extern	int	rm_dmaxlen;
extern	int	rm_b2bc[];

extern	SEARCH_T	**rm_searches;

static	char	fm_emsg[ 256 ];
static	char	*fm_locus;
static	int	fm_comp;
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
static	int	match_helix();
static	int	paired();
static	void	mark_helix();
static	void	unmark_helix();
static	int	chk_wchlx0();
static	int	chk_motif();
static	int	chk_wchlx();

static	void	print_match();
static	void	set_mbuf();

IDENT_T	*find_id();

int	find_motif_driver( n_searches, searches, sites,
	locus, comp, slen, sbuf )
int	n_searches;
SEARCH_T	*searches[];
SITE_T	*sites;
char	locus[];
int	comp;
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

	fm_locus = locus;
	fm_comp = comp;
	fm_slen = slen;
	fm_sbuf = sbuf;

	w_winsize = rm_dmaxlen < fm_windowsize ? rm_dmaxlen : fm_windowsize;

	srp = searches[ 0 ];
	l_szero = slen - w_winsize;
	for( fm_szero = 0; fm_szero < l_szero; fm_szero++ ){
		srp->s_zero = fm_szero;
		srp->s_dollar = fm_szero + w_winsize - 1;
		fm_window[ srp->s_zero - 1 - fm_szero ] = UNDEF;
		fm_window[ srp->s_dollar + 1 - fm_szero ] = UNDEF;
		find_motif( srp );
	}

	l_szero = slen - rm_dminlen;
	srp->s_dollar = slen - 1;
	for( ; fm_szero <= l_szero; fm_szero++ ){
		srp->s_zero = fm_szero;
		find_motif( srp );
	}

	return( 0 );
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
		for( sdollar = f_sdollar; sdollar >= l_sdollar; sdollar-- ){
			srp->s_dollar = sdollar;

			if( n_srp != NULL ){
				n_srp->s_zero = sdollar + 1;
				n_srp->s_dollar = o_sdollar;
			}else{

			}

			rv = find_1_motif( srp );
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

	return( rv );
}

static	int	find_ss( srp )
SEARCH_T	*srp;
{
	STREL_T	*stp, *n_stp;
	SEARCH_T	*n_srp;
	int	s, slen, szero, sdollar;

	stp = srp->s_descr;
	szero = srp->s_zero;
	sdollar = srp->s_dollar;
	slen = sdollar - szero + 1;
	if( slen < stp->s_minlen || slen > stp->s_maxlen )
		return( 0 );

	if( stp->s_seq != NULL ){
		strncpy( fm_chk_seq, &fm_sbuf[ szero ], slen );
		fm_chk_seq[ slen ] = '\0';
		if( !step( fm_chk_seq, stp->s_expbuf ) )
			return( 0 );
	}

	for( s = 0; s < slen; s++ ) 
		fm_window[ szero + s - fm_szero ] = stp->s_index;
	stp->s_matchoff = szero;
	stp->s_matchlen = slen;

	n_stp = srp->s_forward;
	if( n_stp != NULL ){
		n_srp = rm_searches[ n_stp->s_searchno ];
		if( find_motif( n_srp ) ){
			return( 1 );
		}else{
			for( s = 0; s < slen; s++ ) 
				fm_window[ szero + s - fm_szero ] = UNDEF;
			return( 0 );
		}
	}else{
		if( chk_motif( rm_n_descr, rm_descr ) )
			print_match( stdout, fm_locus, fm_comp,
				rm_n_descr, rm_descr );
	}

	return( 1 );
}

static	int	find_wchlx( srp )
SEARCH_T	*srp;
{
	STREL_T	*stp, *stp3, *i_stp, *n_stp;
	int	s, s3lim, slen, szero, sdollar;
	int	h_minl, h_maxl;
	int	i_minl, i_maxl, i_len;
	int	h, h3, hlen;
	SEARCH_T	*i_srp, *n_srp;

	szero = srp->s_zero;
	sdollar = srp->s_dollar;
	slen = sdollar - szero + 1;
	stp = srp->s_descr;

	h_minl = stp->s_minlen;
	h_maxl = stp->s_maxlen;
	i_minl = stp->s_minilen;
	i_maxl = stp->s_maxilen;

	s3lim = sdollar - szero + 1;
	s3lim = ( s3lim - i_minl ) / 2;
	s3lim = MIN( s3lim, h_maxl );
	s3lim = sdollar - s3lim + 1;

	if( match_helix( stp, szero, sdollar, s3lim, &h3, &hlen ) ){

		if( !chk_wchlx0( srp, szero, h3 ) )
			return( 0 );

		i_len = h3 - szero - 2 * hlen + 1;
		if( i_len > i_maxl )
			return( 0 );

		stp3 = stp->s_mates[ 0 ];
		mark_helix( stp, szero, stp3, h3, hlen );

		i_stp = stp->s_inner;
		i_srp = rm_searches[ i_stp->s_searchno ];
		i_srp->s_zero = szero + hlen;
		i_srp->s_dollar = h3 - hlen;

		if( find_motif( i_srp ) ){
			return( 1 );
		}else{
			unmark_helix( stp, szero, stp3, h3, hlen );
			return( 0 );
		}
	}else
		return( 0 );
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

	if( match_helix( stp, szero, s1_dollar, s13_lim, &h13, &h1len ) ){

		mark_helix( stp, szero, stp2, h13, h1len );

		s2_zero = szero + h1len + stp->s_minilen;
		s20_lim = h13 - h1len - stp1->s_minilen - stp1->s_minlen + 1;
		s23_lim = s1_dollar + stp2->s_minilen + 1;

		for( s2 = s2_zero; s2 <= s20_lim; s2++ ){
			if(match_helix(stp1,s2,sdollar,s23_lim,&h23,&h2len)){

				i1_len = s2 - szero - h1len;
				if( i1_len > i1_maxl ){
					unmark_helix(stp,szero,stp2,h13,h1len);
					return( 0 );
				}
				i2_len = h13 - h1len - ( s2 + h2len ) + 1;
				if( i2_len > i2_maxl ){
					unmark_helix(stp,szero,stp2,h13,h1len);
					return( 0 );
				}
				i3_len = h23 - h13 - h2len;
				if( i3_len > i3_maxl ){
					unmark_helix(stp,szero,stp2,h13,h1len);
					return( 0 );
				}

				mark_helix( stp1, s2, stp3, h23, h2len );

				i1_srp->s_zero = szero + h1len;
				i1_srp->s_dollar = s2 - 1;
				i2_srp->s_zero = s2 + h2len;
				i2_srp->s_dollar = h13 - h1len;
				i3_srp->s_zero = h13 + 1;
				i3_srp->s_dollar = h23 - h2len;

				if( find_motif( i1_srp ) ){
					return( 1 );
				}else{
					unmark_helix(stp,szero,stp2,h13,h1len);
					unmark_helix(stp1,s2,stp3,h23,h2len );
					return( 0 );
				}
			}
		}
	}
	return( 0 );
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
	bpcnt = 1;
	*hlen = 1;
	mpr = 0;
	for( bpcnt = 1, *hlen = 1, s = s3 - 1; s >= s3lim; s--, (*hlen)++ ){

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
	if( bpcnt < stp->s_minlen || bpcnt > stp->s_maxlen )
		return( 0 );

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

static	void	mark_helix( stp5, h5, stp3, h3, hlen )
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

static	void	unmark_helix( stp5, h5, stp3, h3, hlen )
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
		fm_window[ h5+h-fm_szero ] = stp5->s_index;
		fm_window[ h3-h-fm_szero ] = stp5->s_index;
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
	return( !paired( stp, b5, b3 ) );
}

static	int	chk_motif( n_descr, descr )
int	n_descr;
STREL_T	descr[];
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
			break;
		case SYM_T1 :
			break;
		case SYM_Q1 :
			break;

		default :	
			break;
		}
	}
	return( 1 );
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
				if( paired( stp, b5, b3 ) )
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
		if( paired( stp, b5, b3 ) )
			return( 0 );
	}
	return( 1 );
}

static	void	print_match( fp, locus, comp, n_descr, descr )
FILE	*fp;
char	locus[];
int	comp;
int	n_descr;
STREL_T	descr[];
{
	int	d, len;
	STREL_T	*stp;
	char	mbuf[ 256 ];

	for( stp = descr, len = 0, d = 0; d < n_descr; d++, stp++ )
		len += stp->s_matchlen;

	fprintf( fp, "%-12s %d", locus, comp );
	stp = descr; 
	set_mbuf( stp->s_matchoff, stp->s_matchlen, mbuf );
	fprintf( fp, " %7d %4d %s", stp->s_matchoff + 1, len, mbuf );

	for( ++stp, d = 1; d < n_descr; d++, stp++ ){
		if( stp->s_matchlen > 0 ){
			set_mbuf( stp->s_matchoff, stp->s_matchlen, mbuf );
			fprintf( fp, " %s", mbuf );
		}else
			fprintf( fp, " ." );
	}
	fprintf( fp, "\n" );
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
