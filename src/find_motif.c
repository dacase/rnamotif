#include <stdio.h>
#include <string.h>
#include "rnamot.h"
#include "y.tab.h"
#define	RM_R2L(st)	\
	((st)==SYM_P3||(st)==SYM_H3||(st)==SYM_T2||(st)==SYM_Q2||(st)==SYM_Q4)

static	int	strel_name();
static	int	find_start();
static	int	find_start2();
static	int	find_stop();
static	int	find_stop2();
static	void	find_plen();	
static	int	find_nlen();	
static	int	find_start3();
static	int	contains_unbnd();
static	int	min_prefixlen();
static	int	max_prefixlen();
static	int	min_suffixlen();
static	int	max_suffixlen();

int	find_motif( n_descr, descr, sites, locus, slen, sbuf )
int	n_descr;
STREL_T descr[];
SITE_T	*sites;
char	locus[];
int	slen;
char	sbuf[];
{
	char	prefix[ 100 ];
	int	tminlen, tmaxlen;
	
	fprintf( stderr, "locus = %s, slen = %d\n", locus, slen );
	fprintf( stderr,
	"descr minl  maxl  mngl  mxgl  mnil  mxil start  stop  descr\n" );
	strcpy( prefix, "+" );
	print_limits( stderr, slen, 0, prefix, 0, n_descr - 1, n_descr, descr );
}

print_limits( fp, slen, lev, prefix, fd, ld, n_descr, descr )
FILE	*fp;
int	slen;
int	lev;
char	prefix[];
int	fd;
int	ld;
int	n_descr;
STREL_T	descr[];
{
	int	d, nd, s;
	STREL_T	*stp, *stp1, *stp2;
	char	prefix1[ 100 ], prefix2[ 100 ];;

	for( d = fd; d <= ld; d = nd ){
		stp = &descr[ d ];
		print_1_limit( fp, slen, lev, prefix, stp, n_descr, descr );
		ext_prefix( stp, prefix, prefix1 ); 
		for( s = 1; s < stp->s_n_scopes; s++ ){
			stp1 = stp->s_scopes[ s - 1 ];
			stp2 = stp->s_scopes[ s ];
			print_limits( fp, slen, lev+2, prefix1, 
				stp1->s_index+1, stp2->s_index-1,
				n_descr, descr );
			ext_prefix( stp2, prefix, prefix2 ); 
			print_1_limit( fp, slen, lev+1, prefix2, stp2,
				n_descr, descr );
		}
		stp1 = stp->s_next;
		if( stp1 == NULL )
			return;
		else
			nd = stp1->s_index;
	} 
}

print_1_limit( fp, slen, lev, prefix, stp, n_descr, descr )
FILE	*fp;
int	slen;
int	lev;
char	prefix[];
STREL_T	*stp;
int	n_descr;
STREL_T	descr[];
{
	int	start, stop, l2r;
	char	name[ 20 ], tstr[ 20 ];
	char	*bp, buf[ 200 ];

	bp = buf;
	sprintf( bp, "%4d", stp->s_index );
	bp += strlen( bp );

	sprintf( bp, " %5d", stp->s_minlen );
	bp += strlen( bp );

	if( stp->s_maxlen == UNBOUNDED )
		sprintf( bp, " UNBND" );
	else
		sprintf( bp, " %5d", stp->s_maxlen );
	bp += strlen( bp );

	if( stp->s_minglen == UNDEF )
		sprintf( bp, " UNDEF" );
	else
		sprintf( bp, " %5d", stp->s_minglen );
	bp += strlen( bp );

	if( stp->s_maxglen == UNBOUNDED )
		sprintf( bp, " UNBND" );
	else if( stp->s_maxglen == UNDEF )
		sprintf( bp, " UNDEF" );
	else
		sprintf( bp, " %5d", stp->s_maxglen );
	bp += strlen( bp );

	if( stp->s_minilen == UNDEF )
		sprintf( bp, " UNDEF" );
	else
		sprintf( bp, " %5d", stp->s_minilen );
	bp += strlen( bp );

	if( stp->s_maxilen == UNBOUNDED )
		sprintf( bp, " UNBND" );
	else if( stp->s_maxilen == UNDEF )
		sprintf( bp, " UNDEF" );
	else
		sprintf( bp, " %5d", stp->s_maxilen );
	bp += strlen( bp );

/*
	start = find_start2( stp, descr, &l2r );
*/
	start = find_start3( stp, descr, &l2r );
	sprintf( tstr, "%s%d", !l2r ? "$-" : "", start );
	sprintf( bp, " %5s", tstr );
	bp += strlen( bp );

/*
	stop = find_stop( slen, stp, n_descr, descr, &l2r );
*/
	stop = find_stop2( slen, stp, n_descr, descr, &l2r );
	sprintf( tstr, "%s%d", !l2r ? "$-" : "", stop );
	sprintf( bp, " %5s", tstr );
	bp += strlen( bp );

	strel_name( stp, name );
	sprintf( bp, "  %s%s", prefix, name );
	bp += strlen( bp );

	if( stp->s_scope == 0 ){
		sprintf( bp, "+--+" );
		bp += strlen( bp );
	}
	sprintf( bp, "\n" );
	bp += strlen( bp );

	fputs( buf, fp );
}

static	int	strel_name( stp, name )
STREL_T	*stp;
char	name[];
{

	switch( stp->s_type ){
	case SYM_SS :
		strcpy( name, "ss" );
		break;

	case SYM_H5 :
		strcpy( name, "h5" );
		break;
	case SYM_H3 :
		strcpy( name, "h3" );
		break;

	case SYM_P5 :
		strcpy( name, "p5" );
		break;
	case SYM_P3 :
		strcpy( name, "p3" );
		break;

	case SYM_T1 :
		strcpy( name, "t1" );
		break;
	case SYM_T2 :
		strcpy( name, "t2" );
		break;
	case SYM_T3 :
		strcpy( name, "t3" );
		break;

	case SYM_Q1 :
		strcpy( name, "q1" );
		break;
	case SYM_Q2 :
		strcpy( name, "q2" );
		break;
	case SYM_Q3 :
		strcpy( name, "q3" );
		break;
	case SYM_Q4 :
		strcpy( name, "q4" );
		break;
	}
}

static	int	find_start( slen, stp, n_descr, descr, l2r )
int	slen;
STREL_T	*stp;
int	n_descr;
STREL_T	descr[];
int	*l2r;
{
	int	i;
	int	start;
	STREL_T	*stp1;

	if(    	stp->s_type == SYM_SS ||
		stp->s_type == SYM_H5 || stp->s_type == SYM_P5 ||
		stp->s_type == SYM_T1 || stp->s_type == SYM_T3 ||
		stp->s_type == SYM_Q1 || stp->s_type == SYM_Q4 )
	{
		*l2r = 1;
		start = 0;
		for( stp1 = descr, i = 0; i < stp->s_index; i++, stp1++ )
			start += stp1->s_minlen;
		return( start + 1 );
	}else{
		*l2r = 0;
		start = 0;
		for( i = stp->s_index + 1; i < n_descr; i++, stp1++ ){
			stp1 = &descr[ i ];
			start += stp1->s_minlen;
		}
		return( start );
	}
}

static	int	find_stop( slen, stp, n_descr, descr, l2r )
int	slen;
STREL_T	*stp;
int	n_descr;
STREL_T	descr[];
int	*l2r;
{
	int	i, unbnd;
	int	stop;
	STREL_T	*stp1;

	if(    	stp->s_type == SYM_SS ||
		stp->s_type == SYM_H5 || stp->s_type == SYM_P5 ||
		stp->s_type == SYM_T1 || stp->s_type == SYM_T3 ||
		stp->s_type == SYM_Q1 || stp->s_type == SYM_Q4 )
	{
		*l2r = 0;
		stop = 0;
		for( i = stp->s_index + 1; i < n_descr; i++, stp1++ ){
			stp1 = &descr[ i ];
			stop += stp1->s_minlen;
		}
		return( stop );
	}else{
		for( unbnd = 0, i = stp->s_index - 1; i >= 0; i-- ){
			stp1 = &descr[ i ];
			if( stp1->s_maxlen == UNBOUNDED ){
				unbnd = 1;
				break;
			}
		}
		if( unbnd ){
			*l2r = 1;
			stop = 0;
			for( i = stp->s_index - 1; i >= 0; i-- ){
				stp1 = &descr[ i ];
				stop += stp1->s_minlen;
			}
		}else{
			*l2r = 0;
			stop = 0;
			for( i = stp->s_index - 1; i >= 0; i-- ){
				stp1 = &descr[ i ];
				stop += stp1->s_maxlen;;
			}
		}
		return( stop );
	}
}

static	int	find_stop2( slen, stp, n_descr, descr, l2r )
int	slen;
STREL_T	*stp;
int	n_descr;
STREL_T	descr[];
int	*l2r;
{
	int	i, unbnd;
	int	stop;
	STREL_T	*stp1;

	if( RM_R2L( stp->s_type ) ){
		*l2r = 1;
		stop = stp->s_minlen + min_prefixlen( stp, descr ) - 1;
	}else{
		*l2r = 0;
		stop = stp->s_minlen + min_suffixlen( stp, descr );
		if( stop > 0 )
			stop--;
	}
	return( stop );
}

ext_prefix( stp, prefix, prefix1 )
STREL_T	*stp;
char	prefix[];
char	prefix1[];
{
	char	*pp, *pp1;
	int	plen;
	STREL_T	*stp0;
	int	inner, next, first;

	strcpy( prefix1, prefix );
	plen = strlen( prefix1 );
	pp = &prefix1[ plen - 1 ];

	inner = stp->s_inner != NULL;
	if( inner )
		first = stp->s_scope == 0;
	else
		first = 0;
	next = stp->s_next != NULL;
	if( next )
		*pp = '|';
	else if( *pp != '|' )
		*pp = ' ';
	if( first )
		strcat( prefix1, "  |" );
	strcat( prefix1, "  +" );
}

int	find_start2( stp, descr, l2r )
STREL_T	*stp;
STREL_T	descr[];
int	*l2r;
{
	int	start, s;
	int	plen, nlen;
	STREL_T	*stp1, *stp2;
	int	unbnd, s_unbnd, p_unbnd, n_unbnd ;

	s_unbnd = stp->s_maxlen == UNBOUNDED;
	if( RM_R2L( stp->s_type ) ){
		if( s_unbnd ){
			start = 0;
			for( stp1 = stp->s_next; stp1; stp1 = stp1->s_next )
				start += stp1->s_minglen;
		}else{
			find_plen( stp, descr, &p_unbnd, &start );
			if( p_unbnd ){
			}else{
				start += stp->s_maxlen; 
			}
		}
		*l2r = !( s_unbnd || p_unbnd );
		if( *l2r )
			start--;
	}else{
		*l2r = 1;
		p_unbnd = 0;
		for( start = 0, stp1 = stp->s_prev; stp1; stp1 = stp1->s_prev ){
			start += stp1->s_minglen;
			if( stp1->s_maxglen == UNBOUNDED )
				p_unbnd = 1;
		}
		for( s = stp->s_scope - 1; s >= 0; s-- ){
			stp2 = stp->s_scopes[ s ];
			start += stp2->s_minlen + stp2->s_minilen;
			if( stp2->s_maxlen==UNBOUNDED ||
				stp2->s_maxilen==UNBOUNDED )
				unbnd = 1;
		}
	}
	return( start );
}

static	void	find_plen( stp, descr, p_unbnd, plen )
STREL_T	*stp;
STREL_T	descr[];
int	*p_unbnd;
int	*plen;
{
	int	start, s, sl;
	STREL_T	*stp1, *stp2, *stp3; 

	*p_unbnd = 0;
	*plen = 0;
	stp1 = stp->s_outer;
	for( s = stp->s_index - 1; s >= stp1->s_index; s-- ){
		stp2 = &descr[ s ];
		if( stp2->s_maxlen == UNBOUNDED ){
			*p_unbnd = 1;
			break;
		}else
			*plen += stp2->s_maxlen;
	}
	if( !*p_unbnd ){
		for( stp2 = stp1->s_prev; stp2; stp2 = stp2->s_prev ){
			if( stp2->s_maxglen == UNBOUNDED ){
				*p_unbnd = UNBOUNDED;
				break;
			}else
				*plen += stp2->s_maxglen;
		}
	}else{
		*plen = 0;
		if( stp->s_n_scopes > 0 ){
			stp3 = stp->s_scopes[ stp->s_n_scopes - 1 ];
			sl = stp3->s_index;
			for( s = stp->s_index + 1; s <= sl; s++ ){
				stp2 = &descr[ sl ];
				*plen += stp2->s_minlen;
			}
		}
	}
}

find_start3( stp, descr, l2r )
STREL_T	*stp;
STREL_T	descr[];
int	*l2r;
{
	char	name[ 20 ];
	int	start;

	strel_name( stp, name );
	if( stp->s_scope == UNDEF ){	/* ss	*/
		*l2r = 1; 
		start = 0;
	}else if( stp->s_scope == 0 ){	/* start a group	*/
		*l2r = 1; 
		start = 0;
	}else{
		if( RM_R2L( stp->s_type ) ){
			if( contains_unbnd( stp, descr ) ){
				start = min_suffixlen( stp, descr );
				*l2r = 0;
			}else{
				start = max_prefixlen( stp, descr );
				start += stp->s_maxlen - 1;
				*l2r = 1;
			}
		}else{
			start = min_prefixlen( stp, descr );
			*l2r = 1;
		}
	}

	return( start );
}

static	int	contains_unbnd( stp, descr )
STREL_T	*stp;
STREL_T	descr[];
{

	if( stp->s_maxlen == UNBOUNDED )
		return( 1 );
	if( max_prefixlen( stp, descr ) == UNBOUNDED )
		return( 1 );
	return( 0 );
}

static	int	min_prefixlen( stp, descr )
STREL_T	*stp;
STREL_T	descr[];
{
	STREL_T	*stp0, *stp1;
	int	s, plen;

	if( stp->s_scope == UNDEF )
		return( 0 );
	stp0 = stp->s_scopes[ 0 ];
	for( plen = 0, s = stp->s_index - 1; s >= stp0->s_index; s-- ){
		stp1 = &descr[ s ];
		plen += stp1->s_minlen;
	}
	return( plen );
}

static	int	max_prefixlen( stp, descr )
STREL_T	*stp;
STREL_T	descr[];
{
	STREL_T	*stp0, *stp1;
	int	s, plen;

	if( stp->s_scope == UNDEF )
		return( 0 );
	stp0 = stp->s_scopes[ 0 ];
	for( plen = 0, s = stp->s_index - 1; s >= stp0->s_index; s-- ){
		stp1 = &descr[ s ];
		if( stp1->s_maxlen == UNBOUNDED )
			return( UNBOUNDED );
		else
			plen += stp1->s_maxlen;
	}
	return( plen );
}

static	int	min_suffixlen( stp, descr )
STREL_T	*stp;
STREL_T	descr[];
{
	STREL_T	*stp0, *stp1, *stpn;
	int	s, slen;

	slen = 0;
	if( stp->s_scope == UNDEF ){
		for( stp1 = stp->s_next; stp1; stp1 = stp1->s_next )
			if( stp1->s_scope == 0 )
				slen += stp1->s_minglen;
			else
				slen += stp1->s_minlen;
		return( slen );
	}

	slen = 0;
	stpn = stp->s_scopes[ stp->s_n_scopes - 1 ];
	for( s = stp->s_index + 1; s <= stpn->s_index; s++ ){
		stp1 = &descr[ s ];
		slen += stp1->s_minlen;
	}
	stp0 = stp->s_scopes[ 0 ];
	for( stp1 = stp0->s_next; stp1; stp1 = stp1->s_next )
		slen += stp1->s_minlen;
	return( slen );
}

static	int	max_suffixlen( stp, descr )
STREL_T	*stp;
STREL_T	descr[];
{
	STREL_T	*stp0, *stp1, *stpn;
	int	s, slen;

	if( stp->s_scope == UNDEF || stp->s_scope == 0 ){
		for( stp1 = stp->s_next; stp1; stp1 = stp1->s_next ){
			if( stp1->s_maxlen == UNBOUNDED )
				return( UNBOUNDED );
			slen += stp1->s_maxlen;
		}
		return( slen );
	}

	slen = 0;
	stpn = stp->s_scopes[ stp->s_n_scopes - 1 ];
	for( s = stp->s_index + 1; s <= stpn->s_index; s++ ){
		stp1 = &descr[ s ];
		if( stp1->s_maxlen == UNBOUNDED )
			return( UNBOUNDED );
		slen += stp1->s_maxlen;
	}
	stp0 = stp->s_scopes[ 0 ];
	for( stp1 = stp0->s_next; stp1; stp1 = stp1->s_next )
		if( stp1->s_maxlen == UNBOUNDED )
			return( UNBOUNDED );
		slen += stp1->s_maxlen;
	return( slen );
}
