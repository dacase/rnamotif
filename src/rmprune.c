#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "log.h"
#include "split.h"

#define	U_MSG_S	"usage: %s [ rnamotif-out-file ]\n"

#include "rmdefs.h"

#define	MAXFIELDS	200	
static	char	*fields[ MAXFIELDS ];
static	int	n_fields;

#define	LINE_SIZE	50000
static	char	ra_line[ LINE_SIZE ];
static	char	line[ LINE_SIZE ];

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
	int	*b_basepair;
} BLOCK_T;

#define	B_UNDEF		UNDEF
#define	B_SAME		0
#define	B_LEFT		1	
#define	B_DOWN		2
#define	B_DIFF		3

#define	BLOCK_SIZE	1000
static	BLOCK_T	block[ BLOCK_SIZE ];
static	int	n_block;

static	int	wc_only;

static	int	MY_getline( char [], FILE * );
static	void	ungetline( char [] );
static	DESCR_T	*getdescr();
static	int	getname( char [], char [] );
static	void	prune_block( FILE *, int, BLOCK_T [] );
#if 0
static	int	find_0_bulge( int, BLOCK_T [] );
static	void	rm_0_bulge( int, BLOCK_T [] );
#endif
static	void	rezip_group( FILE *, int, int, int, BLOCK_T [] );
static	void	getdetails( BLOCK_T * );
static	int	chkrel( BLOCK_T *, BLOCK_T * );
static	int	wchlxrel( int, DETAIL_T *, DETAIL_T *, DETAIL_T *, DETAIL_T * );
static	int	otherhlxrel( DESCR_T *, int, BLOCK_T *, BLOCK_T * );
static	int	enter_block( FILE *, char [], int, BLOCK_T [] );

int
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
		fputs( line, stdout );
	}

	*l_bname = '\0';
	for( n_block = 0; MY_getline( line, fp ); ){
		if( *line == '#' )
			continue;
		if( *line == '>' ){
			if( !getname( line, bname ) ){
				MY_getline( line, fp );
				continue;
			}
			if( strcmp( l_bname, bname ) ){
				prune_block( stdout, n_block, block );
				n_block = 0;
			}
			if( n_block >= BLOCK_SIZE ){
				prune_block( stdout, n_block, block );
				n_block = 0;
			}
			n_block = enter_block( fp, line, n_block, block );
		}else{
			LOG_ERROR("unexpected input: '%s'", line );
			rval = 1;
			goto CLEAN_UP;
		}
		strcpy( l_bname, bname );
	}
	prune_block( stdout, n_block, block );

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
	int	d, d1, d2, dt;
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

	wc_only = 1;
	for( dp = descr, d = 0; d < *n_descr; d++, dp++ ){
		dt = dp->d_type =
			getdtype( fields[ d+dfield1 ], n_dnames, dnames );
		if( dt != DT_SS && dt != DT_H5 && dt != DT_H3 )
			wc_only = 0;
		for( d1 = 0; d1 < 4; d1++ )
			dp->d_els[ d1 ] = UNDEF;
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

static void	prune_block( FILE *fp, int n_block, BLOCK_T block[] )
{
	int	lb, b, bk, f_comp;
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
				rezip_group( fp, g, 0, b - lb, &block[ lb ] );
				start = bp->b_start;
				stop = bp->b_stop;
				lb = b;
				g++;
			}
		}
		rezip_group( fp, g, 0, b - lb, &block[ lb ] );

		bp = &block[ f_comp ];
		start = bp->b_start;
		stop = bp->b_stop;
		for( g = 0, lb = b = f_comp; b < n_block; b++, bp++ ){
			if( bp->b_start > start || bp->b_stop < stop ){
				rezip_group( fp, g, 1, b - lb, &block[ lb ] );
				start = bp->b_start;
				stop = bp->b_stop;
				lb = b;
				g++;
			}
		}
		rezip_group( fp, g, 1, b - lb, &block[ lb ] );
	}

	for( bp = block, bk = 0, b = 0; b < n_block; b++, bp++ ){
		if( !bp->b_keep ){
			free( bp->b_def );
			free( bp->b_hit );
			free( bp->b_details );
			continue;
		}
		block[ bk ] = block[ b ];
		bk++;
	}
	n_block = bk;

#if 0
	if( n_block > 1 )
		n_block = find_0_bulge( n_block, block );
#endif

	for( bp = block, b = 0; b < n_block; b++, bp++ ){
		fputs( bp->b_def, fp );
		fputs( bp->b_hit, fp );
		free( bp->b_def );
		free( bp->b_hit );
		free( bp->b_details );
	}
}

static void	rezip_group( FILE *fp,
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

#if 0
static	int	find_0_bulge( int n_block, BLOCK_T block[] )
{
	BLOCK_T	*bp;
	int	s_comp, comp;
	int	s_start, start;
	int	s_stop, stop;
	int	b, s_b, nb, bk;

	if( !wc_only || n_block < 2 )
		return( n_block );
	
	s_b = 0;
	nb = 1;
	bp = block;
	s_comp = bp->b_comp;
	s_start = bp->b_start;
	s_stop = bp->b_stop;
	for( b = 1; b < n_block; b++ ){
		bp = &block[ b ];
		comp = bp->b_comp;
		start = bp->b_start;
		stop = bp->b_stop;
		if( s_comp != comp || s_start != start || s_stop != stop ){
			if( nb >  1 )
				rm_0_bulge( nb, &block[ s_b ] );
			s_b = b;
			nb = 1;
			comp = bp->b_comp;
			start = bp->b_start;
			stop = bp->b_stop;
		}else
			nb++;
	}
	if( nb >  1 )
		rm_0_bulge( nb, &block[ s_b ] );

	for( bp = block, bk = 0, b = 0; b < n_block; b++, bp++ ){
		if( !bp->b_keep ){
			free( bp->b_def );
			free( bp->b_hit );
			free( bp->b_details );
			continue;
		}
		block[ bk ] = block[ b ];
		bk++;
	}

	return( bk );
}

static	void	rm_0_bulge( int n_block, BLOCK_T block[] )
{
	int	b, b1, nbp;
	int	dt, d, d5, d3;
	BLOCK_T	*bp, *bp1;
	DETAIL_T	*de, *de5, *de3;
	int	b0, b5, b3;
	int	p;
	int	comp, same;

	bp = block;
	comp = bp->b_comp;
	if( !comp )
		nbp = bp->b_details[ n_descr - 1 ].d_stop -
			bp->b_details[ 0 ].d_start + 1;
	else{
		nbp = bp->b_details[ 0 ].d_stop -
			bp->b_details[ n_descr - 1 ].d_start + 1;
{
	int	d;
	for( d = 0; d < n_descr; d++ )
		fprintf( stderr, "det[%2d]: %3d:%3d\n", d,
		bp->b_details[d].d_start, bp->b_details[d].d_stop );
}
	}

LOG_DEBUG("nbp = %d", nbp );

	for( b = 0; b < n_block; b++, bp++ ){
		bp->b_basepair = ( int * )malloc( nbp * sizeof( int ) );
		if( bp->b_basepair == NULL ){
			LOG_ERROR("can't allocate b_basepair.");
			exit( 1 );
		}
		for( b0 = 0; b0 < nbp; b0++ )
			bp->b_basepair[ b0 ] = UNDEF;
	}

	bp = block;
	comp = bp->b_comp;
	if( comp ){
		b0 = bp->b_details[ 0 ].d_start - nbp + 1;
	}else
		b0 = bp->b_details[ 0 ].d_start;

LOG_DEBUG("b0 = %d", b0 );

	for( bp = block, b = 0; b < n_block; b++, bp++ ){
		for( d = 0; d < n_descr; d++ ){
			dt = descr[ d ].d_type;
			switch( dt ){
			case DT_SS :
				break;
			case DT_H5 :
				d3 = descr[ d ].d_els[ 1 ];
				de = &bp->b_details[ d ];
				de3 = &bp->b_details[ d3 ];
				if( !comp ){
					for(b5=de->d_start;b5<=de->d_stop;b5++){
						b3 = ( de3->d_stop - b0 ) - 
							( b5 - de->d_start );
LOG_DEBUG("b5 = %d, b3 = %d", b5, b3 );
						bp->b_basepair[ b5 - b0 ] = b3;
					}
				}else{
					for(b5=de->d_start;b5>=de->d_stop;b5--){
						b3 = ( de3->d_stop - b0 ) + 
							( b5 - de->d_start );
LOG_DEBUG("b5 = %d, b3 = %d", b5, b3 );
						bp->b_basepair[ b5 - b0 ] = b3;
					}
				}
				break;
			case DT_H3 :
				d5 = descr[ d ].d_els[ 0 ];
				de5 = &bp->b_details[ d5 ];
				de = &bp->b_details[ d ];
				if( !comp ){
					for(b3=de->d_stop;b3>=de->d_start;b3--){
						b5 = ( de5->d_start - b0 ) + 
							( de->d_stop - b3 );
LOG_DEBUG("b3 = %d, b5 = %d", b3, b5 );
						bp->b_basepair[ b3 - b0 ] = b5;
					}
				}else{
					for(b3=de->d_stop;b3<=de->d_start;b3++){
						b5 = ( de5->d_start - b0 ) - 
							( b3 - de->d_stop );
LOG_DEBUG("b3 = %d, b5 = %d", b3, b5 );
						bp->b_basepair[ b3 - b0 ] = b5;
					}
				}
				break;
			default :
				LOG_ERROR("dtype %d not supported.", dt );
				exit( 1 );
				break;
			}
		}
	}

	for( bp = block, b = 0; b < n_block - 1; b++, bp++ ){
		for( bp1 = bp + 1, b1 = b + 1; b1 < n_block; b1++, bp1++ ){
			for( same = 1, p = 0; p < nbp; p++ ){
				if( bp->b_basepair[p] != bp1->b_basepair[p] ){
LOG_DEBUG("b = %d, b1 = %d, p = %3d, diff:", b, b1, p );
					same = 0;
					break;
				}
			}
			if( same ){
				bp->b_keep = 0;
				break;
			}
		}
	}

	for( bp = block, b = 0; b < n_block; b++, bp++ )
		free( bp->b_basepair );

}
#endif

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
		LOG_ERROR("can't allocate space for def line.");
		exit( 1 );
	}
	strcpy( sp, line );
	MY_getline( line, fp );
	len = strlen( line );
	sp1 = ( char * )malloc( ( len + 1 ) * sizeof( char ) );
	if( sp1 == NULL ){
		LOG_ERROR("can't allocate space for hit line.");
		exit( 1 );
	}
	strcpy( sp1, line );
	dp = ( DETAIL_T * )malloc( n_descr * sizeof( DETAIL_T ) );
	if( dp == NULL ){
		LOG_ERROR("can't allocate space for details.");
		exit( 1 );
	}
	bp = &block[ n_block ];
	bp->b_keep = 1;
	bp->b_def = sp;
	bp->b_hit = sp1;
	bp->b_hlen = len;
	bp->b_details = dp;
	bp->b_basepair = NULL; 

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
