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
	int	d;
	STREL_T	*stp;
	char	name[ 20 ];
	int	start, stop;
	int	minl, maxl;
	int	l2r;
	char	tstr[ 20 ];
	
	fprintf( stderr, "locus = %s, slen = %d\n", locus, slen );

	fprintf( stderr, "descr =" );
	for( stp = descr, d = 0; d < n_descr; d++, stp++ ){
		strel_name( stp, name );
		fprintf( stderr, "\t%5s", name );
	}
	fprintf( stderr, "\n" );

	fprintf( stderr, "minl  =" );
	for( minl = 0, stp = descr, d = 0; d < n_descr; d++, stp++ ){
		fprintf( stderr, "\t%5d", stp->s_minlen );
		minl += stp->s_minlen;
	}
	fprintf( stderr, "\t%5d\n", minl );

	fprintf( stderr, "maxl  =" );
	for( maxl = 0, stp = descr, d = 0; d < n_descr; d++, stp++ ){
		if( stp->s_maxlen == UNBOUNDED ){
			fprintf( stderr, "\tUNBND" );
			maxl = UNBOUNDED;
		}else{
			if( maxl != UNBOUNDED )
				maxl += stp->s_maxlen;
			fprintf( stderr, "\t%5d", stp->s_maxlen );
		}
	}
	if( maxl == UNBOUNDED )
		fprintf( stderr, "\tUNBND\n" );
	else
		fprintf( stderr, "\t%5d\n", maxl );

	fprintf( stderr, "first =" );
	for( stp = descr, d = 0; d < n_descr; d++, stp++ ){
		start = find_start( slen, stp, n_descr, descr, &l2r );
		sprintf( tstr, "\%s%d", !l2r ? "$-" : "", start );
		fprintf( stderr, "\t%5s", tstr );
	}
	fprintf( stderr, "\n" );

	fprintf( stderr, "last  =" );
	for( stp = descr, d = 0; d < n_descr; d++, stp++ ){
		stop = find_stop( slen, stp, n_descr, descr );
		fprintf( stderr, "\t%5d", stop );
	}
	fprintf( stderr, "\n" );

	fprintf( stderr, "\n" );
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
		stp->s_type == SYM_Q1 || stp->s_type == SYM_Q4 ||
		stp->s_maxlen != UNBOUNDED )
	{
		*l2r = 1;
		start = 0;
		for( stp1 = descr, i = 0; i < stp->s_index; i++, stp1++ )
			start += stp1->s_minlen;
		return( start + 1 );
	}else{
		*l2r = 0;
		start = 0;
		for( i = stp->s_index + 1; i < stp->s_index; i++, stp1++ ){
			stp1 = &descr[ i ];
			start += stp1->s_minlen;
		}
		return( start );
	}
}

static	int	find_stop( slen, stp, n_descr, descr )
int	slen;
STREL_T	*stp;
int	n_descr;
STREL_T	descr[];
{
	int	i;
	int	stop;
	STREL_T	*stp1;

	for( stop = 0, i = stp->s_index; i < n_descr; i++, stp1++ ){
		stp1 = &descr[ i ];
		stop += stp1->s_minlen;
	}
	if( stp->s_type == SYM_H3 ){
		return(  slen - stop + stp->s_minlen );
	}else
		return(  slen - stop + 1 );
}
