#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "log.h"
#include "split.h"

double	atof();

#define	U_MSG_S	\
"usage: %s [ options ] [ output-type ] [ rnamotif-out-file ]\n\
\n\
options:\n\
\t-help\t\t\tPrint this message\n\
\n\
output-type: (Optional) Use one\n\
	-t rnamotif\t\tNormal ct-format (default)\n\
	-t rnaviz\t\tStrict ct-format for rnaviz input\n\
"

#define	UNDEF	(-1)

#define	MAXFIELDS	200	
static	char	*fields[ MAXFIELDS ];
static	int	n_fields;

#define	LINE_SIZE 50000
static	char	ra_line[ LINE_SIZE ];
static	char	line[ LINE_SIZE ];

#define	OT_RNAMOTIF	0
#define	OT_RNAVIZ	1
static	int	o_type = OT_RNAMOTIF;

#define	DT_CTX	0
#define	DT_SS	1
#define	DT_H5	2
#define	DT_H3	3
#define	DT_P5	4
#define	DT_P3	5
#define	DT_T1	6
#define	DT_T2	7
#define	DT_T3	8
#define	DT_Q1	9
#define	DT_Q2	10
#define	DT_Q3	11
#define	DT_Q4	12

typedef	struct	dn2t_t	{
	char	*d_name;
	int	d_type;
} DN2DT_T;

static	DN2DT_T	dnames[] = {
	{ "ctx", DT_CTX },
	{ "ss", DT_SS },
	{ "h5", DT_H5 },
	{ "h3", DT_H3 },
	{ "p5", DT_P5 },
	{ "p3", DT_P3 },
	{ "t1", DT_T1 },
	{ "t2", DT_T2 },
	{ "t3", DT_T3 },
	{ "q1", DT_Q1 },
	{ "q2", DT_Q2 },
	{ "q3", DT_Q3 },
	{ "q4", DT_Q4 }
};
static	int	n_dnames = sizeof( dnames ) / sizeof( DN2DT_T );

typedef	struct	descr_t	{
	int	d_type;
	int	d_els[ 4 ];
	int	d_off;
	char	*d_start;
	char	*d_stop;	
} DESCR_T;

static	DESCR_T	*descr;
static	int	n_descr;
#define	DFIELD1	2	

static	int	MY_getline( char [], FILE * );
static	void	ungetline( char [] );
static	DESCR_T	*getdescr();
static	int	getdtype();
static	int	getsinfo( char [], int, DESCR_T [],
	char [], float *, int *, int *, int *, char ** );
static	void	wr_ctfile( FILE *, int, DESCR_T [],
	char [], float, int, int, int, char * );
static	int	getpn( DESCR_T *, int, DESCR_T [] );

int
main( int argc, char *argv[] )
{
	int	ac;
	char	*fname;
	float	energy;
	FILE	*fp;
	int	d, f;
	char	sname[ 256 ], *seqp;
	int	comp, off, len;
	int	rval = 0;

	fname = NULL;
	fp = NULL;
	for( ac = 1; ac < argc; ac++ ){
		if( !strcmp( argv[ ac ], "-help" ) ){
			fprintf( stderr, U_MSG_S, argv[ 0 ] );
			rval = 0;
			goto CLEAN_UP;
		}else if( !strcmp( argv[ ac ], "-t" ) ){
			ac++;
			if( ac >= argc ){
				fprintf( stderr, U_MSG_S, argv[ 0 ] );
				rval = 1;
				goto CLEAN_UP;
			}else if( !strcmp( argv[ ac ], "rnamotif" ) ){
				o_type = OT_RNAMOTIF;
			}else if( !strcmp( argv[ ac ], "rnaviz" ) ){
				o_type = OT_RNAVIZ;
			}else{
				fprintf( stderr, U_MSG_S, argv[ 0 ] );
				rval = 1;
				goto CLEAN_UP;
			}
		}else if( *argv[ ac ] == '-' ){
			fprintf( stderr, U_MSG_S, argv[ 0 ] );
			rval = 1;
			goto CLEAN_UP;
		}else if( fname != NULL ){
			fprintf( stderr, U_MSG_S, argv[ 0 ] );
			rval = 1;
			goto CLEAN_UP;
		}else
			fname = argv[ ac ];
	}

	if( fname == NULL )
		fp = stdin;
	else if( ( fp = fopen( fname, "r" ) ) == NULL ){
		LOG_ERROR("can't read rnamotif-out-file %s.", fname );
		rval = 1;
		goto CLEAN_UP;
	}

	while( MY_getline( line, fp ) ){
		if( *line == '>' ){
			ungetline( line );
			break;
		}
		n_fields = split( line, fields, " \t\n" );
		if( n_fields == 0 )
			continue;
		else if( !strcmp( fields[ 0 ], "#RM" ) ){
			if( n_fields < 2 )
				continue;
			if( !strcmp( fields[ 1 ], "descr" ) ){
				descr = getdescr( n_fields, DFIELD1, fields,
					&n_descr );
				if( descr == NULL ){
					rval = 1;
					goto CLEAN_UP;
				}
			}
		}
		for( f = 0; f < n_fields; f++ )
			free( fields[ f ] );
	}

	for( d = 0; d < n_descr; d++ ){
		if( descr[ d ].d_type == DT_T1 ){
			LOG_ERROR("can't make CT file for triple helices.\n");
			rval = 1;
			goto CLEAN_UP;
		}else if( descr[ d ].d_type == DT_Q1 ){
			LOG_ERROR("can't make CT file for quad helices.");
			rval = 1;
			goto CLEAN_UP;
		}
	}

	while( MY_getline( line, fp ) ){
		if( *line == '#' )
			continue;
		if( *line == '>' )
			MY_getline( line, fp );
		if(!getsinfo( line, n_descr, descr,
			sname, &energy, &comp, &off, &len, &seqp ))
		{
			MY_getline( line, fp );
			continue;
		}
		wr_ctfile( stdout, o_type, descr,
			sname, energy, comp, off, len, seqp );
	}

CLEAN_UP : ;

	if( fp != NULL && fp != stdin )
		fclose( fp );

	exit( rval );
}

static	int	MY_getline( char line[], FILE *fp )
{
	int	i, c;
	char	*lp;

	if( *ra_line ){
		strcpy( line, ra_line );
		*ra_line = '\0';
		return( 1 );
	}
	for( lp = line, i = 0; ( c = getc( fp ) ) != EOF; i++ ){
		if( i < LINE_SIZE )
			*lp++ = c;
		if( c == '\n' )
			break;
	}
	*lp = '\0';
	return( lp > line );
}

static	void	ungetline( char line[] )
{

	strcpy( ra_line, line );
}

static	DESCR_T	*getdescr( int n_fields, int dfield1, char *fields[],
	int *n_descr )
{
	DESCR_T	*dp, *dp1, *descr;
	char	*tp, *tp1;
	int	d, d1, d2;
	int	*stk;
	int	stkp, n_els;

	*n_descr = n_fields - dfield1;
	descr = ( DESCR_T * )malloc( *n_descr * sizeof( DESCR_T ) );
	if( descr == NULL ){
		LOG_ERROR("can't allocate descr.");
		return( NULL );
	}
	stk = ( int * )malloc( *n_descr * sizeof( int ) );
	if( stk == NULL ){
		LOG_ERROR("can't allocate stk.");
		return( NULL );
	}
	stkp = 0;

	for( dp = descr, d = 0; d < *n_descr; d++, dp++ ){
		dp->d_type = getdtype( fields[ d+dfield1 ], n_dnames, dnames );
		for( d1 = 0; d1 < 4; d1++ )
			dp->d_els[ d1 ] = UNDEF;
		dp->d_start = NULL;
		dp->d_stop = NULL;
	}
	for( dp = descr, d = 0; d < *n_descr; d++, dp++ ){
		if( dp->d_type == DT_SS )
			continue;
		if((tp = strchr( fields[ d + dfield1 ], '(' ))){
			if( dp->d_els[ 0 ] != UNDEF )
				continue;
			dp->d_els[ 0 ] = d;
			for( n_els = 1, d1 = d + 1; d1 < *n_descr; d1++ ){
				if((tp1=strchr( fields[ d1 + dfield1 ], '(' ))){
					if( !strcmp( tp, tp1 ) ){
						dp->d_els[ n_els ] = d1;
						n_els++;
					}
				}
			}

			for( d1 = 1; d1 < 4 && dp->d_els[ d1 ] != UNDEF; d1++ ){
				dp1 = &descr[ dp->d_els[ d1 ] ];
				for( d2 = 0; d2 < 4; d2++ )
					dp1->d_els[ d2 ] = dp->d_els[ d2 ];
			}

		}
	}

	for( dp = descr, d = 0; d < *n_descr; d++, dp++ ){
		if( dp->d_type == DT_SS )
			continue;
		if( dp->d_els[ 0 ] != UNDEF )
			continue;
		if( dp->d_type == DT_H5 || dp->d_type == DT_P5 ){
			stk[ stkp ] = d;
			stkp++;
		}else if( dp->d_type == DT_H3 || dp->d_type == DT_P3 ){
			stkp--;
			d1 = stk[ stkp ];
			dp1 = &descr[ d1 ];
			dp1->d_els[ 0 ] = d1; dp1->d_els[ 1 ] = d;
			dp ->d_els[ 0 ] = d1; dp ->d_els[ 1 ] = d;
		}
	}

	return( descr );
}

static	int	getdtype( char dname[], int n_dntab, DN2DT_T dntab[] )
{
	int	d;
	DN2DT_T	*dp;

	for( dp = dntab, d = 0; d < n_dntab; d++, dp++ ){
		if( !strncmp( dp->d_name, dname, 2 ) )
			return( dp->d_type );
	}
	return( UNDEF );
}

static	int	getsinfo( char line[], int n_descr, DESCR_T descr[],
	char name[], float *energy, int *comp, int *off, int *len, char **sp )
{
	char	*lp, *np;
	int	d, scnt, llen, ocnt;
	DESCR_T	*dp;

	*name = '\0';
	*energy = 0.0;
	*comp = *off = *len = 0;
	*sp = NULL;
	llen = strlen( line );
	for( scnt = 0, lp = &line[ llen - 2 ]; lp >= line; lp-- ){
		if( isspace( *lp ) )
			scnt++;
		if( scnt >= n_descr ){
			*sp = lp + 1;
			break;
		}
	}

	while( isspace( *lp ) )
		lp--;
	while( isdigit( *lp ) )
		lp--;
	*len = atoi( lp );

	while( isspace( *lp ) )
		lp--;
	while( isdigit( *lp ) )
		lp--;
	*off = atoi( lp );

	while( isspace( *lp ) )
		lp--;
	while( isdigit( *lp ) )
		lp--;
	*comp = atoi( lp );

	for( np = name, lp = line; !isspace( *lp ); lp++ )
		*np++ = *lp;
	*np = '\0';

	*energy = atof( lp );

	ocnt = 0;
	for( dp = descr, lp = *sp, d = 0; d < n_descr; d++, dp++ ){
		while( isspace( *lp ) )
			lp++;
		dp->d_off = ocnt;
		dp->d_start = lp;
		while( !isspace( *lp ) ){
			if( *lp != '.' )
				ocnt++;
			lp++;
		}
		dp->d_stop = lp - 1;
	}

	return( 1 );
}

static	void	wr_ctfile( FILE *fp, int o_type, DESCR_T descr[],
	char name[], float energy, int comp, int off, int len, char *seq )
{
	int	nb;
	char	*sp;
	int	d, d0, d1, bn, pn, hn;
	DESCR_T	*dp;

	for( nb = 0, sp = seq; *sp; sp++ ){
		if( isalpha( *sp ) )
			nb++;
	}
	switch( o_type ){
	case OT_RNAMOTIF :
		fprintf( fp, "%4d %s %d %d %d\n", nb, name, comp, off, len );
		break;
	case OT_RNAVIZ :
		fprintf( fp, "%5d dG = %.3f \"%s %d %d %d\"\n", nb, energy,
			name, comp, off, len );
		break;
	default :
		fprintf( fp, "wr_ctfile: unknown o_type = %d\n", o_type );
		exit( 1 );
	}

	for( d0 = 0, dp = descr, d = 0; d < n_descr; d++, dp++ ){
		if( dp->d_start[ 0 ] == '.' )
			continue;
		for( d1 = 0; d1 < dp->d_stop - dp->d_start + 1; d1++ ){
			bn = d0 + d1 + 1;
			switch( o_type ){
			case OT_RNAMOTIF :
				fprintf( fp, "%4d %c", bn, dp->d_start[ d1 ] );
				fprintf( fp, " %4d", bn - 1 );
				break;
			case OT_RNAVIZ :
				fprintf( fp, "%5d %c", bn,
					toupper( dp->d_start[ d1 ] ) );
				fprintf( fp, " %7d", bn - 1 );
				break;
			}
			fprintf( fp, " %4d", bn == nb ? 0 : bn + 1 );
			pn = getpn( dp, d1, descr );
			fprintf( fp, " %4d", pn );
			hn = comp ? off - bn + 1 : off + bn - 1;
			fprintf( fp, " %4d\n", hn );
		}
		d0 += dp->d_stop - dp->d_start + 1;
	}
}

static	int	getpn( DESCR_T *dp, int d0, DESCR_T descr[] )
{
	DESCR_T	*dp1;
	int	lb;

	lb = dp->d_stop - dp->d_start + 1;
	switch( dp->d_type ){
	case DT_SS :
		return( 0 );
		break;
	case DT_H5 :
		dp1 = &descr[ dp->d_els[ 1 ] ];
		return( dp1->d_off + lb - d0 ); 
		break;
	case DT_H3 :
		dp1 = &descr[ dp->d_els[ 0 ] ];
		return( dp1->d_off + lb - d0 ); 
		break;
	case DT_P5 :
		dp1 = &descr[ dp->d_els[ 1 ] ];
		return( dp1->d_off + d0 ); 
		break;
	case DT_P3 :
		dp1 = &descr[ dp->d_els[ 0 ] ];
		return( dp1->d_off + d0 ); 
		break;
	case DT_T1 :
	case DT_T2 :
	case DT_T3 :
		return( 0 );
		break;
	case DT_Q1 :
	case DT_Q2 :
	case DT_Q3 :
	case DT_Q4 :
		return( 0 );
		break;
	}
	return( -1 );
}
