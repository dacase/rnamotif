/*
 *	This program is a transliteration of the Zuker program efn.f
 *	I have made several obvious modifications in the string handling
 *	of the get data parts. I have also renumbered each array to begin
 *	at C's 0, vs the original FTN's 1.
 *
 *	I have noticed a possible bug in the routine that reads the
 * 	misc loop file, in an if that is always true ...  I have left
 *	it alone.
 *
 *	Since this program is intended for incorporation into rnamotif
 *	which can NOT do circular molecules I have discarded the
 *	bookkeeping that makes circular NA's into linear ones.
 *
 */
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "rnamot.h"

double	atof();

#define	NINT(x)		((int)((x)>=0?(x)+.5:(x)-.5))

#define	WC(i,j)		((i)+(j) == 3)
#define	GU(i,j)		((i)==BCODE_G && (j)==BCODE_T)

extern	char	rm_bc2b[];
#define	N2B(n)	((n)>=BCODE_A&&(n)<=BCODE_T?rm_bc2b[(n)]:'x')

extern	char	rm_efndatadir[];
extern	int	rm_efndataok;
extern	int	rm_l_base;
extern	int	*rm_hstnum;
extern	int	*rm_bcseq;
extern	int	*rm_basepr;

static	char	emsg[ 256 ];

static	int	asint3[ 6 ][ 6 ][ 5 ][ 5 ][ 5 ];
#define	MAX_IBHLOOP	30
static	int	inter[ ( MAX_IBHLOOP + 1 ) ];
static	int	bulge[ ( MAX_IBHLOOP + 1 ) ];
static	int	hairpin[ ( MAX_IBHLOOP + 1 ) ];

static	int	dangle[ 5 ][ 5 ][ 5 ][ 2 ];

static	float	prelog;
static	int	maxpen;
#define	EPARAM_SIZE	16
static	int	eparam[ EPARAM_SIZE ];
#define	POPPEN_SIZE	4
static	int	poppen[ POPPEN_SIZE + 1 ];

static	int	sint2[ 6 ][ 6 ][ 5 ][ 5 ];
static	int	sint4[ 6 ][ 6 ][ 5 ][ 5 ][ 5 ][ 5 ];

#define	MAXTLOOPS	100
static	int	tloops[ MAXTLOOPS ][ 2 ];
static	int	n_tloops;
#define	MAXTRILOOPS	50
static	int	triloops[ MAXTRILOOPS ][ 2 ];
static	int	n_triloops;

static	int	stack[ 5 ][ 5 ][ 5 ][ 5 ];
static	int	tstkh[ 5 ][ 5 ][ 5 ][ 5 ];
static	int	tstki[ 5 ][ 5 ][ 5 ][ 5 ];

#define	STKSIZE	500
static	int	stk[ STKSIZE ][ 3 ];
static	int	stkp;

char	*getenv( char * );

int	RM_getefndata( void );
static	int	gettloops( void );
static	int	gettriloops( void );
static	int	getmiscloop( void );
static	int	getdangle( void );
static	int	getibhloop( void );
static	int	getstack( char [], int [5][5][5][5], int );
static	int	stacktest( char [], int [5][5][5][5] );
static	int	getsymint( void );
static	int	symtest( void );
static	int	getasymint( void );
static	int	packloop( char [] );

static	int	skipto( FILE *, char [], long, char [] );

void	RM_dumpefndata( FILE * );
static	void	dumpdangle( FILE * );
static	void	dumpibhloop( FILE * );
static	void	dumpstack( FILE *, char [], int [5][5][5][5] );
static	void	dumpsint( FILE *fp );

static	int	e_stack( int, int );
static	int	e_ibloop( int, int, int, int );
static	int	e_hploop( int, int );
static	int	e_dangle( int, int, int, int );
static	int	e_aupen( int, int );

void	RM_initst( void );
static	void	push( int, int, int );
static	int	pull( int *, int *, int * );

int	RM_allocefnds( int size )
{

	rm_hstnum = ( int * )malloc( size * sizeof( int ) );
	if( rm_hstnum == NULL ){
		RM_errormsg( 0, "RM_allocefnds: can't allocate rm_hstnum\n" );
		return( 1 );
	}
	rm_bcseq = ( int * )malloc( size * sizeof( int ) );
	if( rm_bcseq == NULL ){
		RM_errormsg( 0, "RM_allocefnds: can't allocate rm_bcseq\n" );
		return( 1 );
	}
	rm_basepr = ( int * )malloc( size * sizeof( int ) );
	if( rm_basepr == NULL ){
		RM_errormsg( 0, "RM_allocefnds: can't allocate rm_basepr\n" );
		return( 1 );
	}

	return( 0 );
}

int	RM_getefndata( void )
{
	int	rval = 1;

	if( *rm_efndatadir == '\0' ){
		RM_errormsg( 0, "RM_getefndata: No efn data directory." );
		return( 0 );
	}

	if( !gettloops() )
		rval = 0;

	if( !gettriloops() )
		rval = 0;

	if( !getmiscloop() )
		rval = 0;

	if( !getdangle() )
		rval = 0;

	if( !getibhloop() )
		rval = 0;

	if( !getstack( "stack.dat", stack, EFN_INFINITY ) )
		rval = 0;
	if( !stacktest( "stack.dat", stack ) )
		rval = 0;

	if( !getstack( "tstackh.dat", tstkh, 0 ) )
		rval = 0;
	if( !stacktest( "tstackh.dat", stack ) )
		rval = 0;

	if( !getstack( "tstacki.dat", tstki, 0 ) )
		rval = 0;
	if( !stacktest( "tstacki.dat", stack ) )
		rval = 0;

	if( !getsymint() )
		rval = 0;
	if( !symtest() )
		rval = 0;

	if( !getasymint() )
		rval = 0;

	return( rval );
}

static	int	gettloops( void )
{
	char	fname[ 256 ];
	FILE	*fp;
	char	line[ 256 ];
	char	loop[ 20 ];
	float	energy;
	int	t, rval;

	sprintf( fname, "%s/%s", rm_efndatadir, "tloop.dat" );
	if( ( fp = fopen( fname, "r" ) ) == NULL ){
		sprintf( emsg, "gettloops: can't read tloops file '%s'.",
			fname );
		RM_errormsg( 0, emsg );
		return( 0 );
	}
	rval = 1;
	if( skipto( fp, "---", sizeof( line ), line ) ){
		for( t = 0; fgets( line, sizeof( line ), fp ); t++ ){
			sscanf( line, "%s %f", loop, &energy );
			if( t < MAXTLOOPS ){
				tloops[ t ][ 0 ] = packloop( loop );
				tloops[ t ][ 1 ] = NINT( 100.0 * energy );
			}
		}
	}else
		rval = 0;
	fclose( fp );

	if( t > MAXTLOOPS ){
		sprintf( emsg,
"gettloops: # of tloops (%d) exceeds MAXTLOOPS (%d), last %d tloops ignored.",
			t, MAXTLOOPS, t - MAXTLOOPS );
		RM_errormsg( 0, emsg );
		n_tloops = MAXTLOOPS;
	}else
		n_tloops = t;

	return( rval );
}

static	int	gettriloops( void )
{
	char	fname[ 256 ];
	FILE	*fp;
	char	line[ 256 ];
	char	loop[ 20 ];
	float	energy;
	int	t, rval;

	sprintf( fname, "%s/%s", rm_efndatadir, "triloop.dat" );
	if( ( fp = fopen( fname, "r" ) ) == NULL ){
		sprintf( emsg, "gettriloops: can't read triloops file '%s'.",
			fname );
		RM_errormsg( 0, emsg );
		return( 0 );
	}
	rval = 1;
	if( skipto( fp, "---", sizeof( line ), line ) ){
		for( t = 0; fgets( line, sizeof( line ), fp ); t++ ){
			sscanf( line, "%s %f", loop, &energy );
			if( t < MAXTRILOOPS ){
				triloops[ t ][ 0 ] = packloop( loop );
				triloops[ t ][ 1 ] = NINT( 100.0 * energy );
			}
		}
	}else
		rval = 0;
	fclose( fp );

	if( t > MAXTRILOOPS ){
		sprintf( emsg,
"gettloops: # of triloops (%d) exceeds MAXTRILOOPS (%d), last %d triloops ignored.",
			t, MAXTRILOOPS, t - MAXTRILOOPS );
		RM_errormsg( 0, emsg );
		n_triloops = MAXTRILOOPS;
	}else
		n_triloops = t;

	return( rval );
}

static	int	getmiscloop( void )
{
	char	fname[ 256 ];
	FILE	*fp;
	char	line[ 256 ];
	float	fv1, fv2, fv3, fv4;
	int	i, rval;

	sprintf( fname, "%s/%s", rm_efndatadir, "miscloop.dat" );
	if( ( fp = fopen( fname, "r" ) ) == NULL ){
		sprintf( emsg, "getmiscloop: can't read miscloop file '%s'.",
			fname );
		RM_errormsg( 0, emsg );
		return( 0 );
	}

	rval = 1;
	if( skipto( fp, "-->", sizeof( line ), line ) ){
		fgets( line, sizeof( line ), fp );
		sscanf( line, "%f", &prelog );
		prelog *= 10.0;
	}else{
		RM_errormsg( 0, "getmiscloop: no prelog." );
		rval = 0;
		goto CLEAN_UP;
	}

	if( skipto( fp, "-->", sizeof( line ), line ) ){
		fgets( line, sizeof( line ), fp );
		sscanf( line, "%f", &fv1 );
		maxpen = NINT( 100.0*fv1 );
	}else{
		RM_errormsg( 0, "getmiscloop: no maxpen." );
		rval = 0;
		goto CLEAN_UP;
	}

	if( skipto( fp, "-->", sizeof( line ), line ) ){
		fgets( line, sizeof( line ), fp );
		sscanf( line, "%f %f %f %f", &fv1, &fv2, &fv3, &fv4 );
		poppen[ 0 ] = 0;
		poppen[ 1 ] = NINT( 100.0*fv1 );
		poppen[ 2 ] = NINT( 100.0*fv2 );
		poppen[ 3 ] = NINT( 100.0*fv3 );
		poppen[ 4 ] = NINT( 100.0*fv4 );
	}else{
		RM_errormsg( 0, "getmiscloop: no poppen values." );
		rval = 0;
		goto CLEAN_UP;
	}

	eparam[ 0 ] = 0;
	eparam[ 1 ] = 0;
	eparam[ 2 ] = 0;
	eparam[ 3 ] = 0;
	eparam[ 6 ] = 30;
	eparam[ 7 ] = 30;

	if( skipto( fp, "-->", sizeof( line ), line ) ){
		fgets( line, sizeof( line ), fp );
		sscanf( line, "%f %f %f", &fv1, &fv2, &fv3 );
		eparam[ 4 ] = NINT( 100.0*fv1 );
		eparam[ 5 ] = NINT( 100.0*fv2 );
		eparam[ 8 ] = NINT( 100.0*fv3 );
	}else{
		RM_errormsg( 0, "getmiscloop: no multibranched loop values." );
		rval = 0;
		goto CLEAN_UP;
	}

	if( !skipto( fp, "-->", sizeof( line ), line ) ){
		for( i = 9; i < EPARAM_SIZE; i++ )
			eparam[ i ] = 0;
	}else{	
		/* these parms are not currently used */
		fgets( line, sizeof( line ), fp );
		sscanf( line, "%f %f %f", &fv1, &fv2, &fv3 );

		if( skipto( fp, "-->", sizeof( line ), line ) ){
			fgets( line, sizeof( line ), fp );
			sscanf( line, "%f", &fv1 );
			eparam[ 9 ] = NINT( 100.0*fv1 );
		}else{
			RM_errormsg( 0,
				"getmiscloop: no terminal AU penalty." );
			rval = 0;
			goto CLEAN_UP;
		}
	
		if( skipto( fp, "-->", sizeof( line ), line ) ){
			fgets( line, sizeof( line ), fp );
			sscanf( line, "%f", &fv1 );
			eparam[ 10 ] = NINT( 100.0*fv1 );
		}else{
			RM_errormsg( 0, "getmiscloop: no GGG hairpin term." );
			rval = 0;
			goto CLEAN_UP;
		}
	
		if( skipto( fp, "-->", sizeof( line ), line ) ){
			fgets( line, sizeof( line ), fp );
			sscanf( line, "%f", &fv1 );
			eparam[ 11 ] = NINT( 100.0*fv1 );
		}else{
			RM_errormsg( 0, "getmiscloop: no c hairpin slope." );
			rval = 0;
			goto CLEAN_UP;
		}
	
		if( skipto( fp, "-->", sizeof( line ), line ) ){
			fgets( line, sizeof( line ), fp );
			sscanf( line, "%f", &fv1 );
			eparam[ 12 ] = NINT( 100.0*fv1 );
		}else{
			RM_errormsg( 0,
				"getmiscloop: no c hairpin intercept." );
			rval = 0;
			goto CLEAN_UP;
		}
	
		if( skipto( fp, "-->", sizeof( line ), line ) ){
			fgets( line, sizeof( line ), fp );
			sscanf( line, "%f", &fv1 );
			eparam[ 13 ] = NINT( 100.0*fv1 );
		}else{
			RM_errormsg( 0,
				"getmiscloop: no c hairpin of 3 term." );
			rval = 0;
			goto CLEAN_UP;
		}
	
		if( skipto( fp, "-->", sizeof( line ), line ) ){
			fgets( line, sizeof( line ), fp );
			sscanf( line, "%f", &fv1 );
			eparam[ 14 ] = NINT( 100.0*fv1 );
		}else{
			RM_errormsg( 0,
			"getmiscloop: no Intermol init free energy." );
			rval = 0;
			goto CLEAN_UP;
		}
	
		if( skipto( fp, "-->", sizeof( line ), line ) ){
			fgets( line, sizeof( line ), fp );
			sscanf( line, "%d", &eparam[ 15 ] );
		}else{
			RM_errormsg( 0, "getmiscloop: no GAIL Rule term." );
			rval = 0;
			goto CLEAN_UP;
		}
	}

CLEAN_UP : ;
	fclose( fp );

	return( rval );
}

static	int	getdangle( void )
{
	char	fname[ 256 ];
	FILE	*fp;
	char	line[ 256 ];
	int	v1, v2, v3, v4;
	char	*fields[ 16 ];
	int	f, n_fields;
	int	rval;

	sprintf( fname, "%s/%s", rm_efndatadir, "dangle.dat" );
	if( ( fp = fopen( fname, "r" ) ) == NULL ){
		sprintf( emsg, "getdangle: can't read dangle file '%s'.",
			fname );
		RM_errormsg( 0, emsg );
		return( 0 );
	}
	rval = 1;
	/* v1:	The last paired base:	5->3
	 * v2:	The first paired base;	3<-5
	 * v3:	The dangling base
	 * v4:	The dangling strand:	0 = 5->3, 1 = 3<-5
	 */
	for( v4 = 0; v4 < 2; v4++ ){
		for( v1 = 0; v1 < 4; v1++ ){
			if( !skipto( fp, "<--", sizeof( line ), line ) ){
				sprintf( emsg,
			"getdangle: premature end of dangle file '%s'\n", 
					fname );
				RM_errormsg( 0, emsg );
				rval = 0;
				goto CLEAN_UP;
			}
			fgets( line, sizeof( line ), fp );
			n_fields = split( line, fields, " \t\n" );
	
			for( f = 0; f < n_fields; f++ ){
				v2 = f / 4;
				v3 = f % 4;
				if( *fields[ f ] != '.' )
					dangle[v1][v2][v3][v4] = 
						NINT( 100.0*atof( fields[f] ) );
				free( fields[ f ] );
			}
		}
	}

CLEAN_UP : ;
	fclose( fp );

	return( rval );
}

static	int	getibhloop( void )
{
	char	fname[ 256 ];
	FILE	*fp;
	char	line[ 256 ];
	char	*fields[ 4 ];
	int	i, f, n_fields;
	int	rval;

	sprintf( fname, "%s/%s", rm_efndatadir, "loop.dat" );
	if( ( fp = fopen( fname, "r" ) ) == NULL ){
		sprintf( emsg, "getibhloop: can't read ibhloop file '%s'.",
			fname );
		RM_errormsg( 0, emsg );
		return( 0 );
	}
	rval = 1;

	if( !skipto( fp, "---", sizeof( line ), line ) ){
		sprintf( emsg, "getibhloop: error in ibhloop file '%s'.",
			fname );
		RM_errormsg( 0, emsg );
		rval = 0;
		goto CLEAN_UP;
	}
	for( i = 1; i <= MAX_IBHLOOP; i++ ){
		fgets( line, sizeof( line ), fp );
		if( i <= MAX_IBHLOOP ){
			n_fields = split( line, fields, " \t\n" );
			if( *fields[ 1 ] == '.' )
				inter[ i ] = EFN_INFINITY;
			else
				inter[ i ] = NINT( 100.0*atof( fields[ 1 ] ) );
			if( *fields[ 2 ] == '.' )
				bulge[ i ] = EFN_INFINITY;
			else
				bulge[ i ] = NINT( 100.0*atof( fields[ 2 ] ) );
			if( *fields[ 3 ] == '.' )
				hairpin[ i ] = EFN_INFINITY;
			else
				hairpin[i] = NINT( 100.0*atof( fields[ 3 ] ) );
			for( f = 0; f < n_fields; f++ )
				free( fields[ f ] );
		}
	}

CLEAN_UP : ;
	fclose( fp );

	return( rval );
}

static	int	getstack( char sfname[], int stack[5][5][5][5], int defval ) 
{
	char	fname[ 256 ];
	FILE	*fp;
	char	line[ 256 ];
	int	v1, v2, v3, v4;
	char	*fields[ 16 ];
	int	f, n_fields;
	int	rval;
	
	sprintf( fname, "%s/%s", rm_efndatadir, sfname );
	if( ( fp = fopen( fname, "r" ) ) == NULL ){
		sprintf( emsg, "getibhloop: can't read stack file '%s'.",
			sfname );
		RM_errormsg( 0, emsg );
		return( 0 );
	}
	rval = 1;

	for( v1 = 0; v1 < 5; v1++ ){
		for( v2 = 0; v2 < 5; v2++ ){
			for( v3 = 0; v3 < 5; v3++ ){
				for( v4 = 0; v4 < 5; v4++ )
					stack[v1][v2][v3][v4] = defval;
			}
		}
	}

	for( v1 = 0; v1 < 4; v1++ ){
		if( !skipto( fp, "<--", sizeof( line ), line ) ){
			sprintf( emsg,
				"getstack: premature end of stack file '%s'.",
				fname );
			RM_errormsg( 0, emsg );
			rval = 0;
			goto CLEAN_UP;
		}
		for( v3 = 0; v3 < 4; v3++ ){
			fgets( line, sizeof( line ), fp );
			n_fields = split( line, fields, " \t\n" );
			for( f = 0; f < n_fields; f++ ){
				v2 = f / 4;
				v4 = f % 4;
				if( *fields[ f ] == '.' )
					stack[v1][v2][v3][v4] = EFN_INFINITY;
				else
					stack[v1][v2][v3][v4] = 
						NINT(100.0*atof( fields[f] ));
				free( fields[ f ] );
			}
		}
	}

CLEAN_UP : ;
	fclose( fp );

	return( rval );
}

static	int	stacktest( char sname[], int stack[5][5][5][5] )
{
	int	rval;
	int	v1, v2, v3, v4;

	for( rval = 1, v1 = 0; v1 < 4; v1++ ){
		for( v2 = 0; v2 < 4; v2++ ){
			for( v3 = 0; v3 < 4; v3++ ){
				for( v4 = 0; v4 < 4; v4++ ){
					if(stack[v1][v2][v3][v4] !=
						stack[v4][v3][v2][v1] ){
						sprintf( emsg,
			"stacktest: stack '%s' symmetry error at %d,%d,%d,%d",
							sname,
							v1, v2, v3, v4 );
						RM_errormsg( 0, emsg );
						rval = 0;
					}
				}
			}
		}
	}
	return( rval );
}

static	int	getsymint( void )
{
	char	fname[ 256 ];
	FILE	*fp;
	char	line[ 256 ];
	int	v1, v2, v3, v4, v5, v6;
	char	*fields[ 24 ];
	int	f, n_fields;
	int	lval, worst;
	int	rval;
	
	sprintf( fname, "%s/%s", rm_efndatadir, "sint2.dat" );
	if( ( fp = fopen( fname, "r" ) ) == NULL ){
		sprintf( emsg, "getsymint: can't read sym-2 loop file '%s'.",
			fname );
		RM_errormsg( 0, emsg );
		return( 0 );
	}
	rval = 1;

	/* Skip the header */	
	if( !skipto( fp, "<--", sizeof( line ), line ) ){
		sprintf( emsg, "getsymint: error in sym-2 loop file '%s'.",
			fname );
		RM_errormsg( 0, emsg );
		rval = 0;
		goto CLEAN_UP;
	}

	for( v1 = 0; v1 < 6; v1++ ){
		if( !skipto( fp, "<--", sizeof( line ), line ) ){
			sprintf( emsg,
			"getsymint: premature end of sym-2 loop file '%s'.",
				fname );
			RM_errormsg( 0, emsg );
			rval = 0;
			goto CLEAN_UP;
		}
		for( v3 = 0; v3 < 4; v3++ ){
			fgets( line, sizeof( line ), fp );
			n_fields = split( line, fields, " \t\n" );
			for( f = 0; f < n_fields; f++ ){
				v2 = f / 4;
				v4 = f % 4;
				lval = NINT( 100.0*atof( fields[ f ] ) );
				sint2[v1][v2][v3][v4] = lval;
				free( fields[ f ] );
			}
		}
	}
	fclose( fp );

	for( v1 = 0; v1 < 6; v1++ ){
		for( v2 = 0; v2 < 6; v2++ ){
			for( worst = -999, v3 = 0; v3 < 4; v3++ ){
				for( v4 = 0; v4 < 4; v4++ )
					worst=MAX(worst, sint2[v1][v2][v3][v4]);
			}
			for( v3 = 0; v3 < 5; v3++ ){
				sint2[v1][v2][v3][ 4] = worst;
				sint2[v1][v2][ 4][v3] = worst;
			}
		}
	}

	sprintf( fname, "%s/%s", rm_efndatadir, "sint4.dat" );
	if( ( fp = fopen( fname, "r" ) ) == NULL ){
		sprintf( emsg, "getsymint: can't read sym-4 loop file '%s'.",
			fname );
		RM_errormsg( 0, emsg );
		return( 0 );
	}

	/* Skip the header */	
	if( !skipto( fp, "<--", sizeof( line ), line ) ){
		sprintf( emsg, "getsymint: error in sym-4 loop file '%s'.",
			fname );
		RM_errormsg( 0, emsg );
		rval = 0;
		goto CLEAN_UP;
	}

	for( v1 = 0; v1 < 6; v1++ ){
	    for( v2 = 0; v2 < 6; v2++ ){
		if( !skipto( fp, "<--", sizeof( line ), line ) ){
		    sprintf( emsg,
			"getsymint: premature end of sym-2 loop file '%s'.",
			fname );
		    RM_errormsg( 0, emsg );
		    rval = 0;
		    goto CLEAN_UP;
		}
		for( v3 = 0; v3 < 4; v3++ ){
		    for( v4 = 0; v4 < 4; v4++ ){
			fgets( line, sizeof( line ), fp );
			n_fields = split(line, fields, " \t\n");
			for( f = 0; f < n_fields; f++ ){
			    v5 = f / 4;
			    v6 = f % 4;
			    lval = NINT( 100.0*atof( fields[f] ) );
			    sint4[v1][v2][v3][v4][v5][v6]= lval;
			    free( fields[ f ] );
			}
		    }
		}
	    }
	}

	for( v1 = 0; v1 < 6; v1++ ){
	    for( v2 = 0; v2 < 6; v2++ ){
		for( worst = -999, v3 = 0; v3 < 4; v3++ ){
		    for( v4 = 0; v4 < 4; v4++ ){
			for( v5 = 0; v5 < 4; v5++ ){
			    for( v6 = 0; v6 < 4; v6++ ){
				worst = MAX( worst,
				    sint4[v1][v2][v3][v4][v5][v6] );
			    }
			}
		    }
		}
		for( v3 = 0; v3 < 5; v3++ ){
		    for( v4 = 0; v4 < 5; v4++ ){
			for( v5 = 0; v5 < 5; v5++ ){
			    sint4[v1][v2][v3][v4][v5][ 4] = worst;
			    sint4[v1][v2][v3][v4][ 4][v5] = worst;
			    sint4[v1][v2][v3][ 4][v4][v5] = worst;
			    sint4[v1][v2][ 4][v3][v4][v5] = worst;
			}
		    }
		}
	    }
	}

CLEAN_UP : ;
	fclose( fp );

	return( rval );
}

static	int	symtest( void )
{
	int	v1, v2, v3, v4, v5, v6, v1a, v2a;
	int	rval;

	for( rval = 1, v1 = 0; v1 < 6; v1++ ){
	    for( v2 = 0; v2 < 6; v2++ ){
		v1a = v1 >= 4 && v1 < 6 ? 9 - v1 : 3 - v1;
		v2a = v2 >= 4 && v2 < 6 ? 9 - v2 : 3 - v2;
		for( v3 = 0; v3 < 4; v3++ ){
		    for( v4 = 0; v4 < 4; v4++ ){
			if( sint2[v1][v2][v3][v4] != sint2[v2a][v1a][v4][v3] ){
			    rval = 0;
			    sprintf( emsg,
"symtest: sint2 failure: sint2[%d][%d][%d][%d] (%d) != sint2[%d][%d][%d][%d] (%d)",
				v1, v2, v3, v4, sint2[v1][v2][v3][v4],
				v2a, v1a, v4, v3, sint2[v2a][v1a][v4][v3] );
			    RM_errormsg( 0, emsg );
			}
		    }
		}
	    }
	}

	for( v1 = 0; v1 < 6; v1++ ){
	    for( v2 = 0; v2 < 6; v2++ ){
		v1a = v1 >= 4 && v1 < 6 ? 9 - v1 : 3 - v1;
		v2a = v2 >= 4 && v2 < 6 ? 9 - v2 : 3 - v2;
		for( v3 = 0; v3 < 4; v3++ ){
		    for( v4 = 0; v4 < 4; v4++ ){
			for( v5 = 0; v5 < 4; v5++ ){
			    for( v6 = 0; v6 < 4; v6++ ){
				if(sint4[v1][v2][v3][v4][v5][v6] !=
				    sint4[v2a][v1a][v6][v5][v4][v3] ){
				    rval = 0;
				    sprintf( emsg, 
"symtest: sint4 failure: sint4[%d][%d][%d][%d][%d][%d] (%d) != sint4[%d][%d][%d][%d][%d][%d] (%d)\n",
					v1, v2, v3, v4, v5, v6,
					sint4[v1][v2][v3][v4][v5][v6],
					v2a, v1a, v6, v5, v4, v3,
					sint4[v2a][v1a][v6][v5][v4][v3] );
				    RM_errormsg( 0, emsg );
				}
			    }
			}
		    }
		}
	    }
	}

	return( rval );
}
static	int	getasymint( void )
{
	char	fname[ 256 ];
	FILE	*fp;
	char	line[ 256 ];
	int	v1, v2, v3, v4, v5;
	char	*fields[ 24 ];
	int	f, n_fields;
	int	lval;
	int	rval;
	
	sprintf( fname, "%s/%s", rm_efndatadir, "asint1x2.dat" );
	if( ( fp = fopen( fname, "r" ) ) == NULL ){
		sprintf( emsg,
			"getasymint: can't read asym-1x2 loop file '%s'.",
			fname );
		RM_errormsg( 0, emsg );
		return( 0 );
	}
	rval = 1;

	/* Skip the header */	
	if( !skipto( fp, "<--", sizeof( line ), line ) ){
		sprintf( emsg, "getasymint: error in asym-1x2 loop file '%s'.",
			fname );
		RM_errormsg( 0, emsg );
		rval = 0;
		goto CLEAN_UP;
	}

	for( v1 = 0; v1 < 6; v1++ ){
	    for( v2 = 0; v2 < 6; v2++ ){
		for( v3 = 0; v3 < 5; v3++ ){
		    for( v4 = 0; v4 < 5; v4++ ){
			for( v5 = 0; v5 < 5; v5++ )
			    asint3[v1][v2][v3][v4][v5] = EFN_INFINITY;
		    }
		}
	    }
	}

	for( v1 = 0; v1 < 6; v1++ ){
	    for( v5 = 0; v5 < 4; v5++ ){
		if( !skipto( fp, "<--", sizeof( line ), line ) ){
			sprintf( emsg,
		"getasymint: premature end of asym-1x2 loop file '%s'.",
				fname );
			RM_errormsg( 0, emsg );
			rval = 0;
			goto CLEAN_UP;
		}
		for( v3 = 0; v3 < 4; v3++ ){
		    fgets( line, sizeof( line ), fp );
		    n_fields = split( line, fields, " \t\n" );
		    for( f = 0; f < n_fields; f++ ){
			v2 = f / 4;
			v4 = f % 4;
			lval = NINT( 100.0*atof( fields[ f ] ) );
			asint3[v1][v2][v3][v4][v5] = lval;
			free( fields[ f ] );
		    }
		}
	    }
	}

CLEAN_UP : ;
	fclose( fp );
	
	return( rval );
}

static	int	packloop( char loop[] )
{
	char	*lp;
	int	num;

	for( lp = loop, num = 0; *lp; lp++ ){
		switch( *lp ){
		case 'A' :
		case 'a' :
			num = ( num << 3 ) + 1;
			break;
		case 'C' :
		case 'c' :
			num = ( num << 3 ) + 2;
			break;
		case 'G' :
		case 'g' :
			num = ( num << 3 ) + 3;
			break;
		case 'T' :
		case 't' :
		case 'U' :
		case 'u' :
			num = ( num << 3 ) + 4;
			break;
		default :
			sprintf( emsg,
				"packloop: illegal char %c (%d)", *lp, *lp );
			RM_errormsg( 1, emsg );
			exit( 1 );
		}
	}
	return( num );
}

static	int	skipto( FILE *fp, char str[], long s_line, char line[] )
{

	while( fgets( line, s_line, fp ) ){
		if( strstr( line, str ) )
			return( 1 );
	}
	return( 0 );
}

void	RM_dumpefndata( FILE *fp )
{

	dumpdangle( fp );
	dumpibhloop( fp );
	dumpstack( fp, "stack", stack );
	dumpstack( fp, "tstackh", tstkh );
	dumpstack( fp, "tstacki", tstki );
	dumpsint( fp );
}

static	void	dumpdangle( FILE *fp )
{
	int	v1, v2, v3, v4;
	float	dval;

	fprintf( fp, "\ndangle:\n" );
	for( v4 = 0; v4 < 2; v4++ ){
		for( v1 = 0; v1 < 4; v1++ ){
			for( v2 = 0; v2 < 4; v2++ ){
				if( v2 > 0 )
					fprintf( fp, "|" );
				for( v3 = 0; v3 < 4; v3++ ){
					fprintf( fp, " %c%c  ", N2B( v1 ),
						v4 == 0 ? N2B( v3 ) : ' ' );
				}
			}
			fprintf( fp, "\n" );
			for( v2 = 0; v2 < 4; v2++ ){
				if( v2 > 0 )
					fprintf( fp, "|" );
				for( v3 = 0; v3 < 4; v3++ ){
					fprintf( fp, " %c%c  ", N2B( v2 ),
						v4 == 1 ? N2B( v3 ) : ' ' );
				}
			}
			fprintf( fp, "\n" );
			for( v2 = 0; v2 < 4; v2++ ){
				if( v2 > 0 )
					fprintf( fp, "|" );
				for( v3 = 0; v3 < 4; v3++ ){
					dval = 0.01*dangle[v1][v2][v3][v4];
					if( dval == 0 )
						fprintf( fp, " .   " );
					else
						fprintf( fp, "%5.2f", dval );
				}
			}
			fprintf( fp, "\n" );
			fprintf( fp, "\n" );
		}
	}
}

static	void	dumpibhloop( FILE *fp )
{
	int	i;

	fprintf( fp, "\nibh:\n" );
	for( i = 1; i <= MAX_IBHLOOP; i++ ){
		fprintf( fp, "%3d", i );
		if( inter[ i ] == EFN_INFINITY )
			fprintf( fp, "   .  " );
		else
			fprintf( fp, " %5.2f", 0.01*inter[ i ] );
		if( bulge[ i ] == EFN_INFINITY )
			fprintf( fp, "   .  " );
		else
			fprintf( fp, " %5.2f", 0.01*bulge[ i ] );
		if( hairpin[ i ] == EFN_INFINITY )
			fprintf( fp, "   .  " );
		else
			fprintf( fp, " %5.2f", 0.01*hairpin[ i ] );
		fprintf( fp, "\n" );
	}
}

static	void	dumpstack( FILE *fp, char sname[], int stack[5][5][5][5] )
{
	int	v1, v2, v3, v4;
	float	dval;

	fprintf( fp, "\nstack '%s':\n", sname );
	for( v1 = 0; v1 < 4; v1++ ){
		for( v3 = 0; v3 < 4; v3++ ){
			if( v3 > 0 )
				fprintf( fp, "|" );
			for( v4 = 0; v4 < 4; v4++ ){
				fprintf( fp, " %cX  ", N2B( v1 ) );
			} 
		}
		fprintf( fp, "\n" );
		for( v3 = 0; v3 < 4; v3++ ){
			if( v3 > 0 )
				fprintf( fp, "|" );
			for( v4 = 0; v4 < 4; v4++ ){
				fprintf( fp, " %cY  ", N2B( v3 ) );
			} 
		}
		fprintf( fp, "\n" );
		for( v3 = 0; v3 < 4; v3++ ){
			for( v2 = 0; v2 < 4; v2++ ){
				if( v2 > 0 )
					fprintf( fp, "|" );
				for( v4 = 0; v4 < 4; v4++ ){
					dval = stack[v1][v2][v3][v4];
					if( dval == EFN_INFINITY )
						fprintf( fp, " .   " );
					else
						fprintf(fp, "%5.2f", 0.01*dval);
							

				} 
			}
			fprintf( fp, "\n" );
		}
	}
}

static	void	dumpsint( FILE *fp )
{
	int	v1, v2, v3, v4;
	int	x1, x2, y1, y2;

	fprintf( fp, "\nsint2:\n" );
	for( v1 = 0; v1 < 6; v1++ ){
		for( v3 = 0; v3 < 4; v3++ ){
			for( v2 = 0; v2 < 6; v2++ ){
				for( v4 = 0; v4 < 4; v4++ ){
					fprintf( fp, "%5.2f",
						.01*sint2[v1][v2][v3][v4] );
				}
				if( v2 < 5 )
					fprintf( fp, "|" );
			}
			fprintf( fp, "\n" );
			if( v3 == 3 )
				fprintf( fp, "\n" );
		}
	}

	fprintf( fp, "\nsint4:\n" );
	for( v1 = 0; v1 < 6; v1++ ){
		for( v2 = 0; v2 < 6; v2++ ){
			for( v3 = 0; v3 < 16; v3++ ){
				x1 = v3 / 4;
				x2 = v3 % 4;
				for( v4 = 0; v4 < 16; v4++ ){
					y1 = v4 / 4;
					y2 = v4 % 4;
					fprintf( fp, "%5.2f",
					.01*sint4[v1][v2][x1][x2][y1][y2] );
				}
				fprintf( fp, "\n" );
				if( v3 == 15 )
					fprintf( fp, "\n" );
			}
		}
	}
}

/* No knots allowed! */
int	RM_knotted( void )
{
	int	i, j, ip, k, l;
	int	rval;

	for( rval = 0, i = 0; i <= rm_l_base; i++ ){
		if( rm_basepr[ i ] != UNDEF ){
			j = MAX( i, rm_basepr[i] );
			ip = MIN( i, rm_basepr[i] );
			if( rm_basepr[ip] != j || rm_basepr[j] != ip ){
				rval = 1;
				sprintf( emsg,
					"Base pair %5d.%5d is not reflexive\n",
					rm_hstnum[ip], rm_hstnum[j] );
				RM_errormsg( 0, emsg );
			}
			for( k = ip+1; k <= j-1; k++ ){
				if( rm_basepr[ k ] != UNDEF ){
					l = rm_basepr[ k ];
					if( l <= ip || l >= j ){
						rval = 1;
						sprintf( emsg,
	"RM_knotted: Base pairs %5d.%5d and %5d.%5d are improperly nested.\n",
						rm_hstnum[ip], rm_hstnum[j],
						rm_hstnum[k], rm_hstnum[l] );
						RM_errormsg( 0, emsg );
					}
				}
			}
		}
	}
	return( rval );
}

/* energy function driver */
int	RM_efn( int i, int j, int open )
{
	int	e;
	int	ip, is, jp, js;
	int	k, kp, sum;

	e = 0;
	if( rm_basepr[i] == UNDEF || rm_basepr[j] == UNDEF ){
		if( open == 0 ){
			while( rm_basepr[i]==UNDEF && rm_basepr[i+1]==UNDEF ){
				i++;
				e += eparam[5];
				if( i >= j-1 )
					return( e );
			}
			while( rm_basepr[j]==UNDEF && rm_basepr[j-1]==UNDEF ){
				j--;
				e += eparam[5];
				if( i >= j-1 )
					return( e );
			}

			if( rm_basepr[i] == UNDEF && rm_basepr[i+1] > i+1 ){
				e += MIN( 0, e_dangle(rm_basepr[i+1],i+1,i,1) )
					+ eparam[5];
				i++;
			}
			if( rm_basepr[j] == UNDEF && rm_basepr[j-1] != UNDEF &&
				rm_basepr[j-1] < j-1 )
			{
				e += MIN( 0, e_dangle(j-1,rm_basepr[j-1],j,0) )
					+ eparam[5];
				j--;
			}
		}else{
			while( rm_basepr[i]==UNDEF && rm_basepr[i+1]==UNDEF ){
				i++;
				if( i >= j-1 )
					return( e );
			}
			while( rm_basepr[j]==UNDEF && rm_basepr[j-1]==UNDEF ){
				j--;
				if( i >= j-1 )
					return( e );
			}

			if( rm_basepr[i] == UNDEF && rm_basepr[i+1] > i+1 ){
				e += MIN( 0, e_dangle(rm_basepr[i+1],i+1,i,1) );
				i++;
			}
			if( rm_basepr[j] == UNDEF && rm_basepr[j-1] != UNDEF &&
				rm_basepr[j-1] < j-1 )
			{
				e += MIN( 0, e_dangle(j-1,rm_basepr[j-1],j,0) );
				j--;
			}
		}
	}

	if( rm_basepr[i] != j ){
		k = rm_basepr[i];
		kp = rm_basepr[j];
		if( k >= kp ){
			sprintf( emsg, "RM_efn: knot: (%5d.%5d) (%5d.%5d)\n",
				rm_hstnum[i], rm_hstnum[k],
				rm_hstnum[kp], rm_hstnum[j] );
			RM_errormsg( 0, emsg );
			return( EFN_INFINITY );
		}
		if( rm_basepr[k+1] != UNDEF ){
			e += RM_efn( i, k, open );
			e += RM_efn( k+1, j, open );
		}else if( rm_basepr[k+2] == UNDEF ){
			e += RM_efn( i, k+1, open );
			e += RM_efn( k+2, j, open );
		}else if(e_dangle( k,i,k+1,0 ) <=
			e_dangle( rm_basepr[k+2],k+2,k+1,1 ))
		{
			e += RM_efn( i, k+1, open );
			e += RM_efn( k+2, j, open );
		}else{
			e += RM_efn( i, k, open );
			e += RM_efn( k+1, j, open );
		}
		return( e );
	}else{
		if( !open )
			e += eparam[8];
		e += e_aupen( i, j );

		for( open = 0; ; ){
			if( rm_basepr[i+1] == j-1 ){
				e += e_stack( i, j );
				i++;
				j--;
				continue;
			}

			for( sum = 0, k = i+1; k < j; ){
				if( rm_basepr[k] > k ){
					sum++;
					ip = k;
					k = rm_basepr[k] + 1;
					jp = k - 1;
					if( k > j ){
						sprintf( emsg,
						"RM_efn: ERROR: %d\n", 51 );
						RM_errormsg( 0, emsg );
						return( EFN_INFINITY );
					}
				}else if( rm_basepr[k] == UNDEF )
					k++;
			}

			if( sum == 0 ){	/* hairpin */
				e += e_hploop( i, j );
				return( e );
			}else if( sum == 1 ){ /* internal or bulge loop */
				e += e_ibloop( i, j, ip, jp );
				i = ip;
				j = jp;
				continue;
			}else{
				is = i + 1;
				js = j - 1;
				e += eparam[4] + eparam[8] + e_aupen( i, j );
				if( rm_basepr[i+1]==UNDEF &&
					rm_basepr[i+2]!=UNDEF )
				{
					if( e_dangle(i,j,i+1,0) <=
						e_dangle(rm_basepr[i+2],
						i+2,i+1,1) )
					{
						is = i + 2;
						e += MIN( 0,
							e_dangle( i,j,i+1,0 )) +
							eparam[5];
					}
				}
				if( rm_basepr[i+1]==UNDEF &&
					rm_basepr[i+2]==UNDEF )
				{
					is = i + 2;
					e += MIN( 0, e_dangle( i,j,i+1,0 ) ) +
						eparam[5];
				}
				if( rm_basepr[j-1]==UNDEF &&
					rm_basepr[j-2]!=UNDEF )
				{
					if( e_dangle(i,j,j-1,1) <=
						e_dangle(j-2,rm_basepr[j-2],
						j-1,0) )
					{
						js = j - 2;
						e += MIN( 0,
							e_dangle( i,j,j-1,1 )) +
							eparam[5];
					}
				}
				if( rm_basepr[j-1]==UNDEF &&
					rm_basepr[j-2]==UNDEF )
				{
					js = j - 2;
					e += MIN( 0, e_dangle( i,j,j-1,1 ) ) +
						eparam[5];
				}
				e += RM_efn( is, js, 0 );
				return( e );
			}
		}
	}
	return( e );
}

/* helical stacking energy */
static	int	e_stack( int i, int j )
{
	int	rval;

	if( i == rm_l_base || j == rm_l_base + 1 )
		return( EFN_INFINITY );

	rval = stack[rm_bcseq[i]][rm_bcseq[j]][rm_bcseq[i+1]][rm_bcseq[j-1]] +
		eparam[0];
	return( rval );
}

/* interior & bulge loop energy */
static	int	e_ibloop( int i, int j, int ip, int jp )
{
	int	size, size1, size2, min4;
	int	lopsid, loginc;
	int	a, b;
	int	lf, rt;
	int	rval;

	if( i<=rm_l_base && ip>rm_l_base || jp<=rm_l_base && j>rm_l_base )
		return( EFN_INFINITY );

	rval = 0;
	size1 = ip - i - 1;
	size2 = j - jp - 1;
	size = size1 + size2;
	min4 = MIN( 4, MIN( size1, size2 ) );

	if( size1 == 0 || size2 == 0 ){ /* bulges */
		if( size == 1 ){
			rval += 
		    stack[rm_bcseq[i]][rm_bcseq[j]][rm_bcseq[ip]][rm_bcseq[jp]]
			    + bulge[size] + eparam[1];
		}else{
			rval += e_aupen( i, j ) + e_aupen( ip, jp );
			if( size > 30 ){
				loginc = NINT( prelog*log( size / 30.0 ) );
				rval += bulge[30] + loginc + eparam[1];
			}else
				rval += bulge[size] + eparam[1];
		}
	}else{	/* internal loops */
		lopsid = fabs( ( double )( size1 - size2 ) );
		if( size > 30 ){			/* BIG loops	*/
			loginc = NINT( prelog*log( size / 30. ) );
			if( ( size1 == 1 || size2 == 1 ) && eparam[ 15 ] == 1 ){
				rval +=
			tstki[rm_bcseq[i]][rm_bcseq[j]][BCODE_A][BCODE_A] +
			tstki[rm_bcseq[jp]][rm_bcseq[ip]][BCODE_A][BCODE_A]+
					inter[30] + loginc + eparam[2] +
					MIN( maxpen, lopsid*poppen[min4] );
			}else{
				rval +=
	tstki[rm_bcseq[i]][rm_bcseq[j]][rm_bcseq[i+1]][rm_bcseq[j-1]] +
	tstki[rm_bcseq[jp]][rm_bcseq[ip]][rm_bcseq[jp+1]][rm_bcseq[ip-1]] +
					inter[30] + loginc + eparam[2] +
					MIN( maxpen, lopsid*poppen[min4] );
			}
		}else if( lopsid == 1 && size == 3 ){	/* 2x1 loops	*/
			if( size1 < size2 ){
				if( WC( rm_bcseq[i], rm_bcseq[j] ) )
					a = rm_bcseq[i];
				else if( GU( rm_bcseq[i], rm_bcseq[j] ) )
					a = 4;
				else if( GU( rm_bcseq[j], rm_bcseq[i] ) ) 
					a = 5;
				if( WC( rm_bcseq[ip], rm_bcseq[jp] ) )
					b = rm_bcseq[ip];
				else if( GU( rm_bcseq[ip], rm_bcseq[jp] ) )
					b = 4;
				else if( GU( rm_bcseq[jp], rm_bcseq[ip] ) ) 
					b = 5;
				if( size == 3 ){ /* allow exp. for 3x2 loops */
					rval += eparam[2] +
		asint3[a][b][rm_bcseq[i+1]][rm_bcseq[j-1]][rm_bcseq[jp+1]];
				}
			}else{
				if( WC( rm_bcseq[jp], rm_bcseq[ip] ) )
					a = rm_bcseq[jp];
				else if( GU( rm_bcseq[jp], rm_bcseq[ip] ) )
					a = 4;
				else if( GU( rm_bcseq[ip], rm_bcseq[jp] ) ) 
					a = 5;
				if( WC( rm_bcseq[j], rm_bcseq[i] ) )
					b = rm_bcseq[j];
				else if( GU( rm_bcseq[j], rm_bcseq[i] ) )
					b = 4;
				else if( GU( rm_bcseq[i], rm_bcseq[j] ) ) 
					b = 5;
				if( size == 3 ){ /* allow exp. for 3x2 loops */
					rval += eparam[2] +
		asint3[a][b][rm_bcseq[jp+1]][rm_bcseq[ip-1]][rm_bcseq[i+1]];
				}
			}
		}else if( lopsid == 0 && size <= 4 ){	/* 1x1, 2x2 loops */
			if( WC( rm_bcseq[i], rm_bcseq[j] ) )
				lf = rm_bcseq[i];
			else if( GU( rm_bcseq[i], rm_bcseq[j] ) ||
				GU( rm_bcseq[j], rm_bcseq[i] ) )
				lf = rm_bcseq[i] + 2;
			else
				return( EFN_INFINITY );

			if( WC( rm_bcseq[ip], rm_bcseq[jp] ) )
				rt = rm_bcseq[ip];
			else if( GU( rm_bcseq[ip], rm_bcseq[jp] ) ||
				GU( rm_bcseq[jp], rm_bcseq[ip] ) )
				rt = rm_bcseq[ip] + 2;
			else
				return( EFN_INFINITY );
			if( size == 2 ){
				rval += eparam[2] +
				    sint2[lf][rt][rm_bcseq[i+1]][rm_bcseq[j-1]];
			}else if( size == 4 ){
				rval += eparam[2] +
sint4[lf][rt][rm_bcseq[i+1]][rm_bcseq[j-1]][rm_bcseq[ip-1]][rm_bcseq[jp+1]];
			}
		}else{					/* 3x2 loops & up */
			if( ( size1 == 1 || size2 == 1 ) && eparam[15] == 1 ){
				rval += eparam[2] +
			tstki[rm_bcseq[i]][rm_bcseq[j]][BCODE_A][BCODE_A] +
			tstki[rm_bcseq[jp]][rm_bcseq[ip]][BCODE_A][BCODE_A]+
					inter[size>30?30:size] +
					MIN( maxpen, lopsid*poppen[min4] );
			}else{
				rval += eparam[2] +
	tstki[rm_bcseq[i]][rm_bcseq[j]][rm_bcseq[i+1]][rm_bcseq[j-1]] +
	tstki[rm_bcseq[jp]][rm_bcseq[ip]][rm_bcseq[jp+1]][rm_bcseq[ip-1]] +
					inter[size>30?30:size] +
					MIN( maxpen, lopsid*poppen[min4] );
			}
		}
	}
	
	return( rval );
}

/* hairpin energy */
static	int	e_hploop( int i, int j )
{
	int	size, ccnt, k;
	int	key, lval, loginc;
	int	rval;

	if( i <= rm_l_base && j > rm_l_base )
		return( EFN_INFINITY );

	rval = 0;
	size = j - i - 1;

	/* poly c loop: */
	for( ccnt = 0, k = i + i; k < j; k++ ){
		if( rm_bcseq[k] == BCODE_C )
			ccnt++;
		else
			break;
	}
	if( ccnt == size ){
		rval = ( size == 3 ) ? eparam[13] : eparam[12]+size*eparam[11];
	}

	/* ggg loop */
	if( i > 1 && j <= rm_l_base ){
		if( rm_bcseq[i] == BCODE_G &&
			rm_bcseq[i-1] == BCODE_G &&
			rm_bcseq[i-2]==BCODE_G &&
			rm_bcseq[j] == BCODE_T )
		{
			rval += eparam[10];
		}
	}

	if( size <= 3 ){ /* loops of 1-3 */
		if( size == 3 ){
			key = rm_bcseq[i+size+1];
			for( k = size; k >= 0; k-- )
				key = ( key << 3 ) + rm_bcseq[i+k];
			for( lval = 0, k = 0; k < n_triloops; k++ ){
				if( triloops[k][0] == key ){
					lval = triloops[k][1];
					break;
				}
			}
		}else
			lval = 0;
		rval += hairpin[size] + eparam[3] + e_aupen(i,j) + lval;
	}else if( size <= 30 ){ /* loops of 4-30 */
		if( size == 4 ){
			key = rm_bcseq[i+size+1];
			for( k = size; k >= 0; k-- )
				key = ( key << 3 ) + rm_bcseq[i+k];
			for( lval = 0, k = 0; k < n_tloops; k++ ){
				if( tloops[k][0] == key ){
					lval = tloops[k][1];
					break;
				}
			}
		}else
			lval = 0;
		rval +=
		tstkh[rm_bcseq[i]][rm_bcseq[j]][rm_bcseq[i+1]][rm_bcseq[j-1]] +
			hairpin[size] + eparam[3] + lval;
	}else{	/* BIG (>30) loops */
		loginc = NINT( prelog*log( size / 30.0 ) );
		rval +=
		tstkh[rm_bcseq[i]][rm_bcseq[j]][rm_bcseq[i+1]][rm_bcseq[j-1]] +
			hairpin[30] + loginc + eparam[3];
	}

	return( rval );
}

/* dangling base energy */
static	int	e_dangle( int i, int j, int ip, int jp )
{
	int	rval;

	rval = dangle[ rm_bcseq[i] ][ rm_bcseq[j] ][ rm_bcseq[ip] ][ jp ]; 
	return( rval );
}

static	int	e_aupen( int i, int j )
{
	static	int	pval[5][5] = {
		{ 0, 0, 0, 1, 0 },
		{ 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 1, 0 },
		{ 1, 0, 1, 0, 0 },
		{ 0, 0, 0, 0, 0 } };
	int	rval = 0;

	rval = pval[ rm_bcseq[i] ][ rm_bcseq[j] ] * eparam[ 9 ];
	return( rval );
}

void	RM_initst( void )
{

	stkp = 0;
}

static	void	push( int a, int b, int c )
{

	stkp++;
	if( stkp >= STKSIZE ){
		RM_errormsg( 1, "push: stack overflow." );
		exit( 1 );
	}
	stk[stkp][0] = a;
	stk[stkp][1] = b;
	stk[stkp][2] = c;
}

static	int	pull( int *a, int *b, int *c )
{

	if( stkp == 0 )
		return( 1 );
	*a = stk[stkp][0];
	*b = stk[stkp][1];
	*c = stk[stkp][2];
	stkp--;
	return( 0 );
}
