#include <stdio.h>
#include <string.h>
#include "rnamot.h"
#include "y.tab.h"

#define	RM_R2L(st)	\
	((st)==SYM_P3||(st)==SYM_H3||(st)==SYM_T2||(st)==SYM_Q2||(st)==SYM_Q4)

static	void	print_limits();
static	void	print_1_limit();
static	void	mk_prefix();
static	void	mk_strel_name();
static	void	find_limits();
static	void	find_1_limit();
static	void	find_start();
static	void	find_stop();
static	int	closes_unbnd();
static	int	min_prefixlen();
static	int	max_prefixlen();
static	int	min_suffixlen();

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
	
	find_limits( 0, 0, descr );
	fprintf( stderr,
	"descr minl  maxl  mngl  mxgl  mnil  mxil start  stop  descr\n" );
	strcpy( prefix, "+" );
	print_limits( stderr, 0, prefix, 0, descr );
	fprintf( stderr, "locus = %s, slen = %d\n", locus, slen );
}

static	void	print_limits( fp, lev, prefix, fd, descr )
FILE	*fp;
int	lev;
char	prefix[];
int	fd;
STREL_T	descr[];
{
	int	d, nd, s;
	STREL_T	*stp, *stp1, *stp2;
	char	prefix1[ 100 ], prefix2[ 100 ];;

	for( d = fd; ; d = nd ){
		stp = &descr[ d ];
		print_1_limit( fp, prefix, stp );
		mk_prefix( stp, prefix, prefix1 ); 
		for( s = 1; s < stp->s_n_scopes; s++ ){
			stp1 = stp->s_scopes[ s - 1 ];
			stp2 = stp->s_scopes[ s ];
			print_limits( fp, lev+2, prefix1, 
				stp1->s_index+1, descr );
			mk_prefix( stp2, prefix, prefix2 ); 
			print_1_limit( fp, prefix2, stp2 );
		}
		stp1 = stp->s_next;
		if( stp1 == NULL )
			return;
		else
			nd = stp1->s_index;
	} 
}

static	void	print_1_limit( fp, prefix, stp )
FILE	*fp;
char	prefix[];
STREL_T	*stp;
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

	sprintf( tstr, "%s%d", !stp->s_start.a_l2r ? "$-" : "",
		stp->s_start.a_offset );
	sprintf( bp, " %5s", tstr );
	bp += strlen( bp );

	sprintf( tstr, "%s%d", !stp->s_stop.a_l2r ? "$-" : "",
		stp->s_stop.a_offset );
	sprintf( bp, " %5s", tstr );
	bp += strlen( bp );

	mk_strel_name( stp, name );
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

static	void	mk_prefix( stp, prefix, prefix1 )
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

static	void	mk_strel_name( stp, name )
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

static	void	find_limits( lev, fd, descr )
int	lev;
int	fd;
STREL_T	descr[];
{
	int	d, nd, s;
	STREL_T	*stp, *stp1, *stp2;

	for( d = fd; ; d = nd ){
		stp = &descr[ d ];
		find_1_limit( lev, stp, descr );
		for( s = 1; s < stp->s_n_scopes; s++ ){
			stp1 = stp->s_scopes[ s - 1 ];
			stp2 = stp->s_scopes[ s ];
			find_limits( lev+2, stp1->s_index+1, descr );
			find_1_limit( lev+1, stp2, descr );
		}
		stp1 = stp->s_next;
		if( stp1 == NULL )
			return;
		else
			nd = stp1->s_index;
	} 
}

static	void	find_1_limit( lev, stp, descr )
int	lev;
STREL_T	*stp;
STREL_T	descr[];
{
	int	start, stop, l2r;
	char	name[ 20 ], tstr[ 20 ];
	char	*bp, buf[ 200 ];

	find_start( stp, descr );
	find_stop( stp, descr );
}

static	void	find_start( stp, descr )
STREL_T	*stp;
STREL_T	descr[];
{
	int	start;

	if( stp->s_scope == UNDEF ){	/* ss	*/
/*
		*l2r = 1; 
		start = 0;
*/
		stp->s_start.a_l2r = 1;
		stp->s_start.a_offset = 0;
	}else if( stp->s_scope == 0 ){	/* start a group	*/
/*
		*l2r = 1; 
		start = 0;
*/
		stp->s_start.a_l2r = 1;
		stp->s_start.a_offset = 0;
	}else{
		if( RM_R2L( stp->s_type ) ){
			if( closes_unbnd( stp, descr ) ){
/*
				start = min_suffixlen( stp, descr );
				*l2r = 0;
*/
				stp->s_start.a_offset =
					min_suffixlen( stp, descr );
				stp->s_start.a_l2r = 0;
			}else{
/*
				start = max_prefixlen( stp, descr );
				start += stp->s_maxlen - 1;
				*l2r = 1;
*/
				stp->s_start.a_offset =
					max_prefixlen( stp, descr );
				stp->s_start.a_offset += stp->s_maxlen - 1;
				stp->s_start.a_l2r = 1;
			}
		}else{
/*
			start = min_prefixlen( stp, descr );
			*l2r = 1;
*/
			stp->s_start.a_offset = min_prefixlen( stp, descr );
			stp->s_start.a_l2r = 1;
		}
	}
}

static	void	find_stop( stp, descr )
STREL_T	*stp;
STREL_T	descr[];
{
	int	i, unbnd;
	int	stop;
	STREL_T	*stp1;

	if( RM_R2L( stp->s_type ) ){
		stp->s_stop.a_offset =
			stp->s_minlen + min_prefixlen( stp, descr ) - 1;
		stp->s_stop.a_l2r = 1;
	}else{
		stp->s_stop.a_offset = 
			stp->s_minlen + min_suffixlen( stp, descr );
		if( stp->s_stop.a_offset > 0 )
			stp->s_stop.a_offset--;
		stp->s_stop.a_l2r = 0;
	}
}

static	int	closes_unbnd( stp, descr )
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
