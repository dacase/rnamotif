#include <stdio.h>

#include "rnamot.h"
#include "y.tab.h"

static	int	strel_name();

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
	
	fprintf( stderr, "locus = %s, slen = %d\n", locus, slen );

	fprintf( stderr, "descr =" );
	for( stp = descr, d = 0; d < n_descr; d++, stp++ ){
		strel_name( stp, name );
		fprintf( stderr, "\t%s", name );
	}
	fprintf( stderr, "\n" );

	fprintf( stderr, "minl  =" );
	for( stp = descr, d = 0; d < n_descr; d++, stp++ ){
		fprintf( stderr, "\t%d", stp->s_minlen );
	}
	fprintf( stderr, "\n" );

	fprintf( stderr, "maxl  =" );
	for( stp = descr, d = 0; d < n_descr; d++, stp++ ){
		if( stp->s_maxlen == UNBOUNDED )
			fprintf( stderr, "\tUNBND", stp->s_maxlen );
		else
			fprintf( stderr, "\t%d", stp->s_maxlen );
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
