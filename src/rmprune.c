#include <stdio.h>
#include <ctype.h>
#include <string.h>

#define	U_MSG_S	"usage: %s [ rnamotif-out-file ]\n"

#define	UNDEF	(-1)

#define	MAXFIELDS	200	
static	char	*fields[ MAXFIELDS ];
static	int	n_fields;

#define	LINE_SIZE	50000
static	char	ra_line[ LINE_SIZE ];
static	char	line[ LINE_SIZE ];

#define	DT_SS	0
#define	DT_H5	1
#define	DT_H3	2
#define	DT_P5	3
#define	DT_P3	4
#define	DT_T1	5
#define	DT_T2	6
#define	DT_T3	7
#define	DT_Q1	8
#define	DT_Q2	9
#define	DT_Q3	10
#define	DT_Q4	11

typedef	struct	dn2t_t	{
	char	*d_name;
	int	d_type;
} DN2DT_T;

static	DN2DT_T	dnames[] = {
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
static	int	getdtype();

typedef	struct	descr_t	{
	int	d_type;
	int	d_els[ 4 ];
} DESCR_T;

static	DESCR_T	*descr;
static	int	n_descr;
#define	DFIELD1	2	

typedef	struct	detail_t	{
	int	d_start;
	int	d_stop;
} DETAIL_T;

typedef	struct	block_t {
	int	b_keep;
	char	*b_def;
	char	*b_hit;
	int	b_hlen;
	int	b_comp;
	int	b_start;
	int	b_stop;
	DETAIL_T	*b_details;
} BLOCK_T;

#define	B_UNDEF		(-1)
#define	B_SAME		0
#define	B_LEFT		1	
#define	B_DOWN		2
#define	B_DIFF		3

#define	BLOCK_SIZE	1000
static	BLOCK_T	block[ BLOCK_SIZE ];
static	int	n_block;

static	int	getline( char [], FILE * );
static	void	ungetline( char [] );
static	DESCR_T	*getdescr();
static	int	getname( char [], char [] );
static	void	analyze_block( FILE *, int, BLOCK_T [] );
static	void	analyze_group( FILE *, int, int, int, BLOCK_T [] );
static	void	getdetails( BLOCK_T * );
static	int	chkrel( BLOCK_T *, BLOCK_T * );
static	int	wchlxrel( int, DETAIL_T *, DETAIL_T *, DETAIL_T *, DETAIL_T * );
static	int	otherhlxrel( DESCR_T *, int, BLOCK_T *, BLOCK_T * );
static	int	enter_block( FILE *, char [], int, BLOCK_T [] );

main( int argc, char *argv[] )
{
	int	ac;
	char	*fname;
	FILE	*fp;
	int	f;
	char	bname[ 256 ], l_bname[ 256 ];
	int	rval = 0;

	fname = NULL;
	fp = NULL;
	for( ac = 1; ac < argc; ac++ ){
		if( fname != NULL ){
			fprintf( stderr, U_MSG_S, argv[ 0 ] );
			rval = 1;
			goto CLEAN_UP;
		}else
			fname = argv[ ac ];
	}

	if( fname == NULL )
		fp = stdin;
	else if( ( fp = fopen( fname, "r" ) ) == NULL ){
		fprintf( stderr, "%s: can't read rnamotif-out-file %s.\n",
			argv[ 0 ], fname );
		rval = 1;
		goto CLEAN_UP;
	}

	while( getline( line, fp ) ){
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
		fputs( line, stdout );
	}

	*l_bname = '\0';
	for( n_block = 0; getline( line, fp ); ){
		if( *line == '#' )
			continue;
		if( *line == '>' ){
			if( !getname( line, bname ) ){
				getline( line, fp );
				continue;
			}
			if( strcmp( l_bname, bname ) ){
				analyze_block( stdout, n_block, block );
				n_block = 0;
			}
			if( n_block >= BLOCK_SIZE ){
				analyze_block( stdout, n_block, block );
				n_block = 0;
			}
			n_block = enter_block( fp, line, n_block, block );
		}else{
			fprintf( stderr, "%s: unexpected input: '%s'\n",
				argv[ 0 ], line );
			rval = 1;
			goto CLEAN_UP;
		}
		strcpy( l_bname, bname );
	}
	analyze_block( stdout, n_block, block );

CLEAN_UP : ;

	if( fp != NULL && fp != stdin )
		fclose( fp );

	exit( rval );
}

static	int	getline( char line[], FILE *fp )
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
		fprintf( stderr, "getdescr: can't allocate descr.\n" );
		return( NULL );
	}
	stk = ( int * )malloc( *n_descr * sizeof( int ) );
	if( stk == NULL ){
		fprintf( stderr, "getdescr: can't allocate stk.\n" );
		return( NULL );
	}
	stkp = 0;

	for( dp = descr, d = 0; d < *n_descr; d++, dp++ ){
		dp->d_type = getdtype( fields[ d+dfield1 ], n_dnames, dnames );
		for( d1 = 0; d1 < 4; d1++ )
			dp->d_els[ d1 ] = UNDEF;
	}
	for( dp = descr, d = 0; d < *n_descr; d++, dp++ ){
		if( dp->d_type == DT_SS )
			continue;
		if( tp = strchr( fields[ d + dfield1 ], '(' ) ){
			if( dp->d_els[ 0 ] != UNDEF )
				continue;
			dp->d_els[ 0 ] = d;
			for( n_els = 1, d1 = d + 1; d1 < *n_descr; d1++ ){
				if( tp1=strchr( fields[ d1 + dfield1 ], '(' ) ){
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
		}else{
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

static	int	getname( char line[], char name[] )
{
	char	*lp, *np;

	*name = '\0';
	if( *line != '>' )
		return( 0 );
	for( lp = &line[ 1 ]; *lp && isspace( *lp ); lp++ )
		;
	if( !*lp )
		return( 0 );
	for( np = name; *lp && *lp != '.' && !isspace( *lp ); lp++ )
		*np++ = *lp;
	*np = '\0';
	return( 1 );
}

static void	analyze_block( FILE *fp, int n_block, BLOCK_T block[] )
{
	int	lb, b, f_comp;
	int	g;
	int	start, stop;
	BLOCK_T	*bp;

	if( n_block == 0 )
		return;

	if( n_block > 1 ){
		for( bp = block, f_comp = 0; f_comp < n_block; f_comp++, bp++ ){
			if( bp->b_comp )
				break;
		}

		bp = block;
		start = bp->b_start;
		stop = bp->b_stop;
		for( g = 0, lb = b = 0; b < f_comp; b++, bp++ ){
			if( bp->b_start < start || bp->b_stop > stop ){
				analyze_group( fp, g, 0, b - lb, &block[ lb ] );
				start = bp->b_start;
				stop = bp->b_stop;
				lb = b;
				g++;
			}
		}
		analyze_group( fp, g, 0, b - lb, &block[ lb ] );

		bp = &block[ f_comp ];
		start = bp->b_start;
		stop = bp->b_stop;
		for( g = 0, lb = b = f_comp; b < n_block; b++, bp++ ){
			if( bp->b_start > start || bp->b_stop < stop ){
				analyze_group( fp, g, 1, b - lb, &block[ lb ] );
				start = bp->b_start;
				stop = bp->b_stop;
				lb = b;
				g++;
			}
		}
		analyze_group( fp, g, 1, b - lb, &block[ lb ] );
	}

	for( bp = block, b = 0; b < n_block; b++, bp++ ){
		if( bp->b_keep ){
			fputs( bp->b_def, fp );
			fputs( bp->b_hit, fp );
		}
		free( bp->b_def );
		free( bp->b_hit );
		free( bp->b_details );
	}
}

static void	analyze_group( FILE *fp,
	int g, int comp, int n_group, BLOCK_T block[] )
{ 
	int	b, b1;
	int	brel;
	BLOCK_T	*bp, *bp1;

	if( n_group < 2 )
		return;

	for( bp = block, b = 0; b < n_group; b++, bp++ )
		getdetails( bp );

	for( b = n_group - 1; b > 0; b-- ){
		bp = &block[ b ];
		if( !bp->b_keep )
			continue;
		for( brel = B_UNDEF, b1 = b - 1; b1 >= 0; b1-- ){
			bp1 = &block[ b1 ];
			if( !bp1->b_keep )
				continue;
			brel = chkrel( bp, bp1 );
			switch( brel ){
			case B_UNDEF :
				break;
			case B_SAME :
				break;
			case B_DOWN :
				bp1->b_keep = 0;
				break;
			case B_LEFT :
				bp->b_keep = 0;
				goto NEXT_b;
				break;
			case B_DIFF :
				break;
			}
		}
NEXT_b : ;
	}
}

static void	getdetails( BLOCK_T *bp )
{
	int	i, len, tlen;
	char	*sp, *sp1;
	DETAIL_T	*dp;

	for( i = 0, sp = &bp->b_hit[ bp->b_hlen -1 ]; sp >= bp->b_hit; sp-- ){
		if( *sp == ' ' )
			i++;
		if( i == n_descr ){
			sp++;
			break;
		}
	}

	tlen = len = 0;
	for( dp = bp->b_details, i = 0; *sp; i++, dp++ ){
		sp1 = strpbrk( sp, " \n" );
		len = sp1 - sp;
		if( !bp->b_comp ){
			dp->d_start = bp->b_start + tlen;
			dp->d_stop = dp->d_start + len - 1;
		}else{
			dp->d_start = bp->b_start - tlen;
			dp->d_stop = dp->d_start - len + 1;
		}
		sp = sp1 + strspn( sp1, " \n" );
		tlen += len;
	}
}

static int	chkrel( BLOCK_T *bp, BLOCK_T *bp1 )
{
	int	d;
	DETAIL_T *dp, *dp_2;
	DETAIL_T *dp1, *dp1_2;
	int	brel, brel1;

	brel = B_UNDEF; 
	for( d = 0; d < n_descr; d++ ){
		switch( descr[ d ].d_type ){
		case DT_H5 :
			dp = &bp->b_details[ d ];
			dp_2 = &bp->b_details[ descr[ d ].d_els[ 1 ] ];
			dp1 = &bp1->b_details[ d ];
			dp1_2 = &bp1->b_details[ descr[ d ].d_els[ 1 ] ];
			brel1 = wchlxrel( bp->b_comp, dp, dp_2, dp1, dp1_2 );
			break;
		case DT_P5 :
		case DT_T1 :
		case DT_Q1 :
			brel1 = otherhlxrel( &descr[ d ], bp->b_comp, bp, bp1 );
			break;
		case DT_SS :
		case DT_H3 :
		case DT_P3 :
		case DT_T2 :
		case DT_T3 :
		case DT_Q2 :
		case DT_Q3 :
		case DT_Q4 :
			brel1 = B_SAME;
			break;
		}
		if( brel1 == B_DIFF )
			return( B_DIFF );
		else if( brel == B_UNDEF )
			brel = brel1;
		else if( brel == B_SAME )
			brel = brel1;
		else if( brel == B_DOWN ){
			if( brel1 == B_LEFT )
				return( B_DIFF );
		}else if( brel == B_LEFT ){
			if( brel1 == B_DOWN )
				return( B_DIFF );
		}
	}
	return( brel );
}

static int	wchlxrel( int comp, DETAIL_T *dp, DETAIL_T *dp_2,
	DETAIL_T *dp1, DETAIL_T *dp1_2 )
{
	int	lod, rod;
	int	lid, rid;

	if( !comp ){
		lod = dp1->d_start - dp->d_start;
		rod = dp_2->d_stop - dp1_2->d_stop;
		if( lod != rod )
			return( B_DIFF );
		lid = dp->d_stop - dp1->d_stop;
		rid = dp1_2->d_start - dp_2->d_start;
		if( lid != rid )
			return( B_DIFF );
	}else{
		lod = dp->d_start - dp1->d_start; 
		rod = dp1_2->d_stop - dp_2->d_stop;
		if( lod != rod )
			return( B_DIFF );
		lid = dp1->d_stop - dp->d_stop;
		rid = dp_2->d_start - dp1_2->d_start;
		if( lid != rid )
			return( B_DIFF );
	}
	if( lod > 0 ){
		if( lid < 0 )
			return( B_DIFF );
		else
			return( B_DOWN );
	}else if( lod == 0 ){
		if( lid < 0 )
			return( B_LEFT );
		else if( lid == 0 )
			return( B_SAME );
		else
			return( B_DOWN );
	}else if( lid < 0 )
		return( B_DIFF );
	else
		return( B_LEFT );
}

static int otherhlxrel( DESCR_T *dep, int comp, BLOCK_T *bp, BLOCK_T *bp1 )
{
	int	e, i;
	DETAIL_T	*dp, *dp1; 

	for( i = 0; i < 4; i++ ){
		if( ( e = dep->d_els[ i ] ) == UNDEF )
			break;
		dp = &bp->b_details[ e ];
		dp1 = &bp1->b_details[ e ];
		if( dp->d_start != dp1->d_start )
			return( B_DIFF );
		else if( dp->d_stop != dp1->d_stop )
			return( B_DIFF );
	}
	return( B_SAME );
}

static int	enter_block( FILE *fp, char line[],
	int n_block, BLOCK_T block[] )
{
	BLOCK_T	*bp;
	char	*sp, *sp1;
	int	scnt, len;
	DETAIL_T	*dp;

	sp = ( char * )malloc( strlen( line ) + 1 );
	if( sp == NULL ){
		fprintf( stderr,
			"enter_block: can't allocate space for def line.\n" );
		exit( 1 );
	}
	strcpy( sp, line );
	getline( line, fp );
	len = strlen( line );
	sp1 = ( char * )malloc( ( len + 1 ) * sizeof( char ) );
	if( sp1 == NULL ){
		fprintf( stderr,
			"enter_block: can't allocate space for hit line.\n" );
		exit( 1 );
	}
	strcpy( sp1, line );
	dp = ( DETAIL_T * )malloc( n_descr * sizeof( DETAIL_T ) );
	if( dp == NULL ){
		fprintf( stderr,
			"enter_block: can't allocate space for details.\n" );
		exit( 1 );
	}
	bp = &block[ n_block ];
	bp->b_keep = 1;
	bp->b_def = sp;
	bp->b_hit = sp1;
	bp->b_hlen = len;
	bp->b_details = dp;

	for( scnt = 0, sp1 = &line[ len - 1 ]; sp1 >= line; sp1-- ){
		if( *sp1 == ' ' )
			scnt++;
		if( scnt == n_descr )
			break;
	}
	for( --sp1; isdigit( *sp1 ); sp1-- )
		;
	for( ; *sp1 == ' '; sp1-- )
		;
	for( --sp1; isdigit( *sp1 ); sp1-- )
		;
	for( ; *sp1 == ' '; sp1-- )
		;
	sscanf( sp1, "%d %d %d",
		&bp->b_comp, &bp->b_start, &bp->b_stop );
	if( !bp->b_comp )
		bp->b_stop = bp->b_start + bp->b_stop - 1;
	else
		bp->b_stop = bp->b_start - bp->b_stop + 1;
	n_block++;
	return( n_block );
}
