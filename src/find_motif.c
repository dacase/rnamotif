#include <stdio.h>

#include "rnamot.h"
#include "y.tab.h"

static	int	strel_name();
static	int	find_start();
static	int	find_stop();

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
	
	find_tlen( stderr, 1, 0, descr, &tminlen, &tmaxlen );
	fprintf( stderr, "tminlen/tmaxlen = %3d/", tminlen );
	if( tmaxlen == UNBOUNDED )
		fprintf( stderr, "UNBND\n" ); 
	else
		fprintf( stderr, "%3d\n", tmaxlen );

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
	char	name[ 20 ], tstr[ 20 ];;

	fprintf( fp, "%4d", stp->s_index );
	fprintf( fp, " %5d", stp->s_minlen );
	if( stp->s_maxlen == UNBOUNDED )
		fprintf( fp, " UNBND" );
	else
		fprintf( fp, " %5d", stp->s_maxlen );
	if( stp->s_minglen == UNDEF )
		fprintf( fp, " UNDEF" );
	else
		fprintf( fp, " %5d", stp->s_minglen );
	if( stp->s_maxglen == UNBOUNDED )
		fprintf( fp, " UNBND" );
	else if( stp->s_maxglen == UNDEF )
		fprintf( fp, " UNDEF" );
	else
		fprintf( fp, " %5d", stp->s_maxglen );
	if( stp->s_minilen == UNDEF )
		fprintf( fp, " UNDEF" );
	else
		fprintf( fp, " %5d", stp->s_minilen );
	if( stp->s_maxilen == UNBOUNDED )
		fprintf( fp, " UNBND" );
	else if( stp->s_maxilen == UNDEF )
		fprintf( fp, " UNDEF" );
	else
		fprintf( fp, " %5d", stp->s_maxilen );
	start = find_start( slen, stp, n_descr, descr, &l2r );
	sprintf( tstr, "%s%d", !l2r ? "$-" : "", start );
	fprintf( fp, " %5s", tstr );
	stop = find_stop( slen, stp, n_descr, descr, &l2r );
	sprintf( tstr, "%s%d", !l2r ? "$-" : "", stop );
	fprintf( fp, " %5s", tstr );
	strel_name( stp, name );
	fprintf( fp, "  %s%s", prefix, name );
	if( stp->s_scope == 0 )
		fprintf( fp, "+--+" );
	fprintf( fp, "\n" );
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

ext_prefix( stp, prefix, prefix1 )
STREL_T	*stp;
char	prefix[];
char	prefix1[];
{
	char	*pp, *pp1;
	int	plen;
	STREL_T	*stp0;
	int	inner, next, first;

/*
fprintf( stderr, "%3d: prefix  = '%s'\n", stp->s_index, prefix );
*/
	strcpy( prefix1, prefix );
	plen = strlen( prefix1 );
	pp = &prefix1[ plen - 1 ];

	inner = stp->s_inner != NULL;
	if( inner )
		first = stp->s_scope == 0;
	else
		first = 0;
	next = stp->s_next != NULL;
/*
	*pp = next ? '|' : ' ';
*/
	if( next )
		*pp = '|';
	else if( *pp != '|' )
		*pp = ' ';
	if( first )
		strcat( prefix1, "  |" );
	strcat( prefix1, "  +" );
/*
fprintf( stderr, "index = %d, inner = %d next = %d first = %d\n",
	stp->s_index, inner, next, first );
fprintf( stderr, "%3d: prefix1 = '%s'\n", stp->s_index, prefix1 );
*/
}

find_tlen( fp, lev, fd, descr, tminlen, tmaxlen )
FILE	*fp;
int	lev;
int	fd;
STREL_T	descr[];
int	*tminlen;
int	*tmaxlen;
{
	int	d, d1, d2, nd;
	int	gminlen, gmaxlen;
	int	minlen2, minlen3;
	int	maxlen2, maxlen3;
	int	nlev;
	STREL_T	*stp, *stp1, *stp2, *stp3;

	*tminlen = 0;
	*tmaxlen = 0;
	for( d = fd; ; d = nd ){
		stp = &descr[ d ];
		gminlen = stp->s_minlen;
		gmaxlen = stp->s_maxlen;
		for( d1 = 0; d1 < stp->s_n_scopes - 1; d1++ ){
			stp1 = stp->s_scopes[ d1 ];
			stp2 = stp->s_scopes[ d1+1 ];
			if( stp1->s_inner ){
				stp3 = stp1->s_inner;
				find_tlen( fp, lev+2, stp3->s_index,
					descr, &minlen3, &maxlen3 );
				stp1->s_minilen = minlen3;
				stp1->s_maxilen = maxlen3;
				gminlen += minlen3;
				if( gmaxlen != UNBOUNDED ){
					if( maxlen3 == UNBOUNDED )
						gmaxlen = UNBOUNDED;
					else
						gmaxlen += maxlen3;
				}
			}
			find_len( fp, lev+1, stp2, &minlen2, &maxlen2 );
			gminlen += minlen2;
			if( gmaxlen != UNBOUNDED ){
				if( maxlen2 == UNBOUNDED )
					gmaxlen = UNBOUNDED;
				else
					gmaxlen += maxlen2;
			}
		} 
		stp->s_minglen = gminlen;
		stp->s_maxglen = gmaxlen;
		*tminlen += gminlen;
		if( *tmaxlen != UNBOUNDED ){
			if( gmaxlen == UNBOUNDED )
				*tmaxlen = UNBOUNDED;
			else
				*tmaxlen += gmaxlen; 
		}
		if( stp->s_next )
			nd = stp->s_next->s_index;
		else
			break;
	} 
}

find_len( fp, lev, stp, minlen, maxlen )
FILE	*fp;
int	lev;
STREL_T	*stp;
int	*minlen;
int	*maxlen;
{
	int	i;

	*minlen = stp->s_minlen;
	*maxlen = stp->s_maxlen;
}
