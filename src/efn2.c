#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

double	atof();

#include "split.h"
#include "rmdefs.h"
#include "rnamot.h"

#define EFN2_INFINITY	9999999	/* an arbitrary value given to infinity	*/
#define	EFN2_MAXIBHLOOP	30	/* max internal, bulge, hairpin loops	*/
#define EFN2_MAXTLOOP	100	/* max 4-loops allowed (info from tloop)*/
#define EFN2_MAXTRILOOP	100	/* max 3-loops allowed (info from tloop)*/

#define	NINT(x)		((int)((x)>=0?(x)+.5:(x)-.5))

/*this structure contains all the info read from*/
/* thermodynamic data files			*/
typedef	struct	efn2data_t	{
	int	e2_inter  [ EFN2_MAXIBHLOOP+1 ];
	int	e2_bulge  [ EFN2_MAXIBHLOOP+1 ];
	int	e2_hairpin[ EFN2_MAXIBHLOOP+1 ];

	int	e2_dangle[ N_BCODES ][ N_BCODES ][ N_BCODES ][ 2 ];

	float	e2_prelog;
	int	e2_maxpen;
	int	e2_poppen[5];
	int	e2_eparam[11];
 	int	e2_efn2a;
	int	e2_efn2b;
	int	e2_efn2c;
	int	e2_auend;
	int	e2_gubonus;
	int	e2_cslope;
	int	e2_cint;
	int	e2_c3;
	int	e2_init;
	int	e2_gail;

	int	e2_iloop21[ N_BCODES ][ N_BCODES ][ N_BCODES ] [ N_BCODES ]
			[ N_BCODES ][ N_BCODES ][ N_BCODES ];
	int	e2_iloop11[ N_BCODES ][ N_BCODES ][ N_BCODES ][ N_BCODES ]
			[ N_BCODES ][ N_BCODES ];
	int	e2_iloop22[ N_BCODES ][ N_BCODES ][ N_BCODES ][ N_BCODES ]
			[ N_BCODES ][ N_BCODES ][ N_BCODES ][ N_BCODES ];

	int	e2_tloop[ EFN2_MAXTLOOP+1 ][ 2 ];
	int	e2_ntloops;
	int	e2_triloop[ EFN2_MAXTLOOP+1 ][ 2 ];
	int	e2_ntriloops;

	int	e2_stack[ N_BCODES ][ N_BCODES ][ N_BCODES ][ N_BCODES ];
	int	e2_tstkh[ N_BCODES ][ N_BCODES ][ N_BCODES ][ N_BCODES ];
	int	e2_tstki[ N_BCODES ][ N_BCODES ][ N_BCODES ][ N_BCODES ];

      	int	e2_coax      [ N_BCODES ][ N_BCODES ][ N_BCODES ][ N_BCODES ];
	int	e2_tstackcoax[ N_BCODES ][ N_BCODES ][ N_BCODES ][ N_BCODES ];
	int	e2_coaxstack [ N_BCODES ][ N_BCODES ][ N_BCODES ][ N_BCODES ];
	int	e2_tstack    [ N_BCODES ][ N_BCODES ][ N_BCODES ][ N_BCODES ];
	int	e2_tstkm     [ N_BCODES ][ N_BCODES ][ N_BCODES ][ N_BCODES ];

} EFN2DATA_T;

#define	STK_SIZE	51
typedef	struct	stackstruct_t	{
	int	stk[ STK_SIZE ][ 4 ];
	int	sp;
} STACKSTRUCT_T;

#define	MIN(a,b)	((a)<(b)?(a):(b))

static	EFN2DATA_T	efn2data;
static	EFN2DATA_T	*ef2dp = &efn2data;

extern	char	rm_efndatadir[];
extern	int	rm_l_base;
extern	int	*rm_bcseq;
extern	int	*rm_basepr;

extern	int	efn2_nbases;

static	char	emsg[ 256 ];

static	int	bmap[] =
	{ BCODE_A, BCODE_C, BCODE_G, BCODE_T, BCODE_G, BCODE_T };
static	int	rmap[] =
	{ BCODE_T, BCODE_G, BCODE_C, BCODE_A, BCODE_T, BCODE_G };

static	int	getmiscloop( char [] );
static	int	getibhloop( char [] );
static	int	getdangle( char [] );
static	int	getstack( char [],
	int [N_BCODES][N_BCODES][N_BCODES][N_BCODES], int );
static	int	getcoax( char [],
	int [N_BCODES][N_BCODES][N_BCODES][N_BCODES], int );
static	int	gettstack( char [],
	int [N_BCODES][N_BCODES][N_BCODES][N_BCODES],
	int [N_BCODES][N_BCODES][N_BCODES][2], int );
static	int	gettloop( char [] );
static	int	gettriloop( char [] );
static	int	get1x1loop( char [] );
static	int	get2x1loop( char [] );
static	int	get2x2loop( char [] );
static	int	packloop( char [] );
static	int	skipto( FILE *, char [], long, char [] );


#define	MAXHELIX	100
static	int	coax[ MAXHELIX ][ MAXHELIX ], helix[ MAXHELIX ][ 2 ];

static	void	push( STACKSTRUCT_T *, int, int, int, int );
static	void	pop( STACKSTRUCT_T *, int *, int *, int *, int *, int * );
static	int	ef2_stack( int, int, int, int );
static	int	ef2_ibloop( int, int, int, int );
static	int	ef2_hploop( int, int );
static	int	ef2_dangle( int, int, int, int );
static	int	ef2_aupen( int, int );
static	int	ef2_tstkm( int, int, int, int );
static	int	ef2_coax( int, int, int, int );
static	int	ef2_tstackcoax( int, int, int, int );
static	int	ef2_coaxstack( int, int, int, int );

/*Function opens data files to read thermodynamic data		*/
int	RM_getefn2data( void )
{

	if( !getmiscloop( "miscloop.dat" ) )
		return( 0 );

	if( !getibhloop( "loop.dat" ) )
		return( 0 );

	if( !getdangle( "dangle.dat" ) )
		return( 0 );

	if( !getstack( "stack.dat", ef2dp->e2_stack, EFN2_INFINITY ) )
		return( 0 );

	if( !getstack( "tstackh.dat", ef2dp->e2_tstkh, EFN2_INFINITY ) )
		return( 0 );

	if( !getstack( "tstacki.dat", ef2dp->e2_tstki, EFN2_INFINITY ) )
		return( 0 );

	if( !getcoax( "coaxial.dat", ef2dp->e2_coax, EFN2_INFINITY ) )
		return( 0 );

	if( !getstack( "tstackcoax.dat", ef2dp->e2_tstackcoax, 0 ) )
		return( 0 );

	if( !getstack( "coaxstack.dat", ef2dp->e2_coaxstack, 0 ) )
		return( 0 );

	if( !getstack( "tstackm.dat", ef2dp->e2_tstkm, 0 ) )
		return( 0 );

	/* DANGER, DANGER, DANGER! 					*/
	/*	tstack requires dangle and auend be defined!		*/
	if( !gettstack( "tstack.dat", ef2dp->e2_tstack, ef2dp->e2_dangle,
		ef2dp->e2_auend ) )
	{
		return( 0 );
	}

	if( !gettloop( "tloop.dat" ) )
		return( 0 );

	if( !gettriloop( "triloop.dat" ) )
		return( 0 );

	if( !get1x1loop( "int11.dat" ) )
		return( 0 );

	if( !get2x1loop( "int21.dat" ) )
		return( 0 );

	if( !get2x2loop( "int22.dat" ) )
		return( 0 );

	return( 1 );
}

static	int	getmiscloop( char fname[] )
{
	char	pname[ 256 ];
	FILE	*fp;
	char	line[ 256 ];
	float	fv1, fv2, fv3, fv4;
	int	i, rval;

	sprintf( pname, "%s/%s", rm_efndatadir, fname );
	if( ( fp = fopen( pname, "r" ) ) == NULL ){
		sprintf( emsg, "getmiscloop: can't read miscloop file '%s'.",
			pname );
		RM_errormsg( FALSE, emsg );
		return( 0 );
	}

	rval = 1;
	if( skipto( fp, "-->", sizeof( line ), line ) ){
		fgets( line, sizeof( line ), fp );
		sscanf( line, "%f", &ef2dp->e2_prelog );
		ef2dp->e2_prelog *= 10.0;
	}else{
		RM_errormsg( FALSE, "getmiscloop: no prelog." );
		rval = 0;
		goto CLEAN_UP;
	}

	if( skipto( fp, "-->", sizeof( line ), line ) ){
		fgets( line, sizeof( line ), fp );
		sscanf( line, "%f", &fv1 );
		ef2dp->e2_maxpen = NINT( 100.0*fv1 );
	}else{
		RM_errormsg( FALSE, "getmiscloop: no maxpen." );
		rval = 0;
		goto CLEAN_UP;
	}

	if( skipto( fp, "-->", sizeof( line ), line ) ){
		fgets( line, sizeof( line ), fp );
		sscanf( line, "%f %f %f %f", &fv1, &fv2, &fv3, &fv4 );
		ef2dp->e2_poppen[ 0 ] = 0;
		ef2dp->e2_poppen[ 1 ] = NINT( 100.0*fv1 );
		ef2dp->e2_poppen[ 2 ] = NINT( 100.0*fv2 );
		ef2dp->e2_poppen[ 3 ] = NINT( 100.0*fv3 );
		ef2dp->e2_poppen[ 4 ] = NINT( 100.0*fv4 );
	}else{
		RM_errormsg( FALSE, "getmiscloop: no poppen values." );
		rval = 0;
		goto CLEAN_UP;
	}

	ef2dp->e2_eparam[ 0 ] = 0;	/* Place holder for 0,1 array lb */
	ef2dp->e2_eparam[ 1 ] = 0;
	ef2dp->e2_eparam[ 2 ] = 0;
	ef2dp->e2_eparam[ 3 ] = 0;
	ef2dp->e2_eparam[ 4 ] = 0;
	ef2dp->e2_eparam[ 7 ] = 30;
	ef2dp->e2_eparam[ 8 ] = 30;
	ef2dp->e2_eparam[ 9 ] = -500;	/* gets 10th val in efn1	*/

	if( skipto( fp, "-->", sizeof( line ), line ) ){
		fgets( line, sizeof( line ), fp );
		sscanf( line, "%f %f %f", &fv1, &fv2, &fv3 );
		ef2dp->e2_eparam[ 5 ] = NINT( 100.0*fv1 );
		ef2dp->e2_eparam[ 6 ] = NINT( 100.0*fv2 );

/*		use 9th parm in efn1, 10th in efn:
		ef2dp->e2_eparam[ 9 ] = NINT( 100.0*fv3 );
*/

		ef2dp->e2_eparam[ 10 ] = NINT( 100.0*fv3 );
	}else{
		RM_errormsg( FALSE,
			"getmiscloop: no multibranched loop values." );
		rval = 0;
		goto CLEAN_UP;
	}

	if( !skipto( fp, "-->", sizeof( line ), line ) ){
		for( i = 9; i < 11/*EPARAM_SIZE*/; i++ )
			ef2dp->e2_eparam[ i ] = 0;
	}else{
		/* these parms are used only by efn2 */
		fgets( line, sizeof( line ), fp );
		sscanf( line, "%f %f %f", &fv1, &fv2, &fv3 );
		ef2dp->e2_efn2a = NINT( 100.0*fv1 );
		ef2dp->e2_efn2b = NINT( 100.0*fv2 );
		ef2dp->e2_efn2c = NINT( 100.0*fv3 );

		if( skipto( fp, "-->", sizeof( line ), line ) ){
			fgets( line, sizeof( line ), fp );
			sscanf( line, "%f", &fv1 );
/*
			efdp->e2_eparam[ 9 ] = NINT( 100.0*fv1 );
*/
			ef2dp->e2_auend = NINT( 100.0*fv1 );
		}else{
			RM_errormsg( FALSE,
				"getmiscloop: no terminal AU penalty." );
			rval = 0;
			goto CLEAN_UP;
		}
	
		if( skipto( fp, "-->", sizeof( line ), line ) ){
			fgets( line, sizeof( line ), fp );
			sscanf( line, "%f", &fv1 );
/*
			efdp->e2_eparam[ 10 ] = NINT( 100.0*fv1 );
*/
			ef2dp->e2_gubonus = NINT( 100.0*fv1 );
		}else{
			RM_errormsg( FALSE,
				"getmiscloop: no GGG hairpin term." );
			rval = 0;
			goto CLEAN_UP;
		}
	
		if( skipto( fp, "-->", sizeof( line ), line ) ){
			fgets( line, sizeof( line ), fp );
			sscanf( line, "%f", &fv1 );
/*
			efdp->e2_eparam[ 11 ] = NINT( 100.0*fv1 );
*/
			ef2dp->e2_cslope = NINT( 100.0*fv1 );
		}else{
			RM_errormsg( FALSE,
				"getmiscloop: no c hairpin slope." );
			rval = 0;
			goto CLEAN_UP;
		}
	
		if( skipto( fp, "-->", sizeof( line ), line ) ){
			fgets( line, sizeof( line ), fp );
			sscanf( line, "%f", &fv1 );
/*
			efdp->e2_eparam[ 12 ] = NINT( 100.0*fv1 );
*/
			ef2dp->e2_cint = NINT( 100.0*fv1 );
		}else{
			RM_errormsg( FALSE,
				"getmiscloop: no c hairpin intercept." );
			rval = 0;
			goto CLEAN_UP;
		}
	
		if( skipto( fp, "-->", sizeof( line ), line ) ){
			fgets( line, sizeof( line ), fp );
			sscanf( line, "%f", &fv1 );
/*
			efdp->e2_eparam[ 13 ] = NINT( 100.0*fv1 );
*/
			ef2dp->e2_c3 = NINT( 100.0*fv1 );
		}else{
			RM_errormsg( FALSE,
				"getmiscloop: no c hairpin of 3 term." );
			rval = 0;
			goto CLEAN_UP;
		}
	
		if( skipto( fp, "-->", sizeof( line ), line ) ){
			fgets( line, sizeof( line ), fp );
			sscanf( line, "%f", &fv1 );
/*
			efdp->e2_eparam[ 14 ] = NINT( 100.0*fv1 );
*/
			ef2dp->e2_init = NINT( 100.0*fv1 );
		}else{
			RM_errormsg( FALSE,
			"getmiscloop: no Intermol init free energy." );
			rval = 0;
			goto CLEAN_UP;
		}
	
		if( skipto( fp, "-->", sizeof( line ), line ) ){
			fgets( line, sizeof( line ), fp );
/*
			sscanf( line, "%d", &efdp->e2_eparam[ 15 ] );
*/
			sscanf( line, "%d", &ef2dp->e2_gail );
		}else{
			RM_errormsg( FALSE, "getmiscloop: no GAIL Rule term." );
			rval = 0;
			goto CLEAN_UP;
		}
	}

CLEAN_UP : ;
	if( fp != NULL ){
		fclose( fp );
		fp = NULL;
	}

	return( rval );
}

static	int	getibhloop( char fname[] )
{
	char	pname[ 256 ];
	FILE	*fp;
	char	line[ 256 ];
	char	*fields[ 4 ];
	int	i, f, n_fields;
	int	rval;

	sprintf( pname, "%s/%s", rm_efndatadir, fname );
	if( ( fp = fopen( pname, "r" ) ) == NULL ){
		sprintf( emsg, "getibhloop: can't read ibhloop file '%s'.",
			pname );
		RM_errormsg( FALSE, emsg );
		return( 0 );
	}
	rval = 1;

	if( !skipto( fp, "---", sizeof( line ), line ) ){
		sprintf( emsg, "getibhloop: error in ibhloop file '%s'.",
			fname );
		RM_errormsg( FALSE, emsg );
		rval = 0;
		goto CLEAN_UP;
	}
	for( i = 1; i <= EFN2_MAXIBHLOOP; i++ ){
		fgets( line, sizeof( line ), fp );
		if( i <= EFN2_MAXIBHLOOP ){
			n_fields = split( line, fields, " \t\n" );
			if( *fields[ 1 ] == '.' )
				ef2dp->e2_inter[ i ] = EFN2_INFINITY;
			else
				ef2dp->e2_inter[ i ] =
					NINT( 100.0*atof( fields[ 1 ] ) );
			if( *fields[ 2 ] == '.' )
				ef2dp->e2_bulge[ i ] = EFN2_INFINITY;
			else
				ef2dp->e2_bulge[ i ] =
					NINT( 100.0*atof( fields[ 2 ] ) );
			if( *fields[ 3 ] == '.' )
				ef2dp->e2_hairpin[ i ] = EFN2_INFINITY;
			else
				ef2dp->e2_hairpin[i] =
					NINT( 100.0*atof( fields[ 3 ] ) );
			for( f = 0; f < n_fields; f++ )
				free( fields[ f ] );
		}
	}

CLEAN_UP : ;
	if( fp != NULL ){
		fclose( fp );
		fp = NULL;
	}

	return( rval );
}

static	int	getdangle( char fname[] )
{
	char	pname[ 256 ];
	FILE	*fp;
	char	line[ 256 ];
	int	v1, v2, v3, v4;
	int	val;
	char	*fields[ 16 ];
	int	f, n_fields;
	int	rval;

	sprintf( pname, "%s/%s", rm_efndatadir, fname );
	if( ( fp = fopen( pname, "r" ) ) == NULL ){
	    sprintf( emsg, "getdangle: can't read dangle file '%s'.",
		pname );
	    RM_errormsg( FALSE, emsg );
	    return( 0 );
	}
	rval = 1;

	/* efn2 has been extended to deal with X's (ie N's, v=0) and	*/
	/* intermolecular bases I's (ie v = 5)				*/
	/* 0 vs * is 0, 5 vs !0 is inf, 1-4 vs 1-4 is OK		*/
	/* N vs * is 0, I vs !N is inf, A-T vs A-T is OK		*/
	for( v4 = 0; v4 < 2; v4++ ){
	    for( v1 = 0; v1 < N_BCODES; v1++ ){
	    	for( v2 = 0; v2 < N_BCODES; v2++ ){
		     for( v3 = 0; v3 < N_BCODES; v3++ ){
			if( v1 == BCODE_N || v2 == BCODE_N || v3 == BCODE_N )
			    val = 0;
			ef2dp->e2_dangle[v1][v2][v3][v4] = val;
		     }
		}
	    }
	}

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
		     RM_errormsg( FALSE, emsg );
		     rval = 0;
		     goto CLEAN_UP;
		}
		fgets( line, sizeof( line ), fp );
		n_fields = split( line, fields, " \t\n" );
	
		for( f = 0; f < n_fields; f++ ){
		    v2 = f / 4;
		    v3 = f % 4;
		    if( *fields[ f ] != '.' )
			ef2dp->e2_dangle[v1][v2][v3][v4] = 
			    NINT( 100.0*atof( fields[f] ) );
		    else
			ef2dp->e2_dangle[v1][v2][v3][v4] = EFN2_INFINITY;
		    free( fields[ f ] );
		}
	     }
	}

CLEAN_UP : ;
	if( fp != NULL ){
	    fclose( fp );
	    fp = NULL;
	}

	return( rval );
}

static	int	getstack( char sfname[],
	int stack[N_BCODES][N_BCODES][N_BCODES][N_BCODES], int b5val ) 
{
	char	pname[ 256 ];
	FILE	*fp;
	char	line[ 256 ];
	int	v1, v2, v3, v4;
	char	*fields[ 16 ];
	int	f, n_fields;
	int	rval;
	
	sprintf( pname, "%s/%s", rm_efndatadir, sfname );
	if( ( fp = fopen( pname, "r" ) ) == NULL ){
		sprintf( emsg, "getstack: can't read stack file '%s'.",
			pname );
		RM_errormsg( FALSE, emsg );
		return( 0 );
	}
	rval = 1;

	for( v1 = 0; v1 < N_BCODES; v1++ ){
	    for( v2 = 0; v2 < N_BCODES; v2++ ){
		for( v3 = 0; v3 < N_BCODES; v3++ ){
		    for( v4 = 0; v4 < N_BCODES; v4++ ){
			if( v1==BCODE_N||v2==BCODE_N||v3==BCODE_N||v4==BCODE_N )
			    stack[v1][v2][v3][v4] = 0;
			else
			    stack[v1][v2][v3][v4] = 0;
		    }
		}
	    }
	}

	for( v1 = 0; v1 < 4; v1++ ){
	    if( !skipto( fp, "<--", sizeof( line ), line ) ){
			sprintf( emsg,
				"getstack: premature end of stack file '%s'.",
				pname );
			RM_errormsg( FALSE, emsg );
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
			stack[v1][v2][v3][v4] = EFN2_INFINITY;
		    else
			stack[v1][v2][v3][v4] = NINT(100.0*atof( fields[f] ));
		    free( fields[ f ] );
		}
	    }
	}

CLEAN_UP : ;
	if( fp != NULL ){
	    fclose( fp );
	    fp = NULL;
	}

	return( rval );
}

static	int	getcoax( char sfname[],
	int stack[N_BCODES][N_BCODES][N_BCODES][N_BCODES], int b5val ) 
{
	char	pname[ 256 ];
	FILE	*fp;
	char	line[ 256 ];
	int	v1, v2, v3, v4;
	char	*fields[ 16 ];
	int	f, n_fields;
	int	rval;
	
	sprintf( pname, "%s/%s", rm_efndatadir, sfname );
	if( ( fp = fopen( pname, "r" ) ) == NULL ){
	    sprintf( emsg, "getcoax: can't read stack file '%s'.",
		pname );
	    RM_errormsg( FALSE, emsg );
	    return( 0 );
	}
	rval = 1;

	for( v1 = 0; v1 < N_BCODES; v1++ ){
	    for( v2 = 0; v2 < N_BCODES; v2++ ){
		for( v3 = 0; v3 < N_BCODES; v3++ ){
		    for( v4 = 0; v4 < N_BCODES; v4++ ){
			if( v1==BCODE_N||v2==BCODE_N||v3==BCODE_N||v4==BCODE_N )
			    stack[v1][v2][v3][v4] = 0;
			else
			    stack[v1][v2][v3][v4] = 0;
		    }
		}
	    }
	}

	for( v1 = 0; v1 < 4; v1++ ){
	    if( !skipto( fp, "<--", sizeof( line ), line ) ){
		sprintf( emsg, "getcoax: premature end of stack file '%s'.",
		    pname );
		RM_errormsg( FALSE, emsg );
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
			stack[v2][v1][v3][v4] = EFN2_INFINITY;
		    else
			stack[v2][v1][v3][v4] = NINT(100.0*atof( fields[f] ));
		    free( fields[ f ] );
		}
	    }
	}

CLEAN_UP : ;
	if( fp != NULL ){
	    fclose( fp );
	    fp = NULL;
	}

	return( rval );
}

static	int	gettstack( char sfname[],
	int tstack[N_BCODES][N_BCODES][N_BCODES][N_BCODES],
	int dangle[N_BCODES][N_BCODES][N_BCODES][2], int auend )
{
	char	pname[ 256 ];
	FILE	*fp;
	char	line[ 256 ];
	int	v1, v2, v3, v4;
	char	*fields[ 16 ];
	int	f;
	int	rval;

	sprintf( pname, "%s/%s", rm_efndatadir, sfname );
	if( ( fp = fopen( pname, "r" ) ) == NULL ){
	    sprintf( emsg, "gettstack: can't read stack file '%s'.\n",
		pname );
	    RM_errormsg( FALSE, emsg );
	    return( 0 );
	}
	rval = 1;

	for( v1 = 0; v1 < N_BCODES; v1++ ){
	    if( v1 !=BCODE_N ){
			if( !skipto( fp, "<--", sizeof( line ), line ) ){
				sprintf( emsg,
			"gettstack: premature end of tstack file '%s'.\n",
					pname );
				RM_errormsg( FALSE, emsg );
				rval = 0;
				goto CLEAN_UP;
			}
	    }
	    for( v3 = 0; v3 < N_BCODES; v3++ ){
		if( v1 != BCODE_N && v3 != BCODE_N ){
		    fgets( line, sizeof( line ), fp );
		    split( line, fields, " \t\n" );
		}
		for( v2 = 0; v2 < N_BCODES; v2++ ){
		    for( v4 = 0; v4 < N_BCODES; v4++ ){
			if(v1==BCODE_N||v2==BCODE_N||v3==BCODE_N||v4==BCODE_N){
			    tstack[v1][v2][v3][v4]=0;
			}else{
			    f = 4*v2+v4;
			    if( *fields[ f ] == '.' ){
				tstack[v1][v2][v3][v4] = EFN2_INFINITY;
			    }else
				tstack[v1][v2][v3][v4] =
				    NINT(100.0*atof( fields[f] ));
			}
		    }
		}
	    }
	}

CLEAN_UP : ;
	if( fp != NULL ){
	    fclose( fp );
	    fp = NULL;
	}

	return( rval );
}

static	int	gettloop( char fname[] )
{
	char	pname[ 256 ];
	FILE	*fp;
	char	line[ 256 ];
	char	loop[ 20 ];
	float	energy;
	int	t;
	int	rval;

	sprintf( pname, "%s/%s", rm_efndatadir, fname );
	if( ( fp = fopen( pname, "r" ) ) == NULL ){
	    sprintf( emsg, "gettloop: can't read tloop file '%s'.",
		pname );
	    RM_errormsg( FALSE, emsg );
	    return( 0 );
	}
	rval = 1;
	if( skipto( fp, "---", sizeof( line ), line ) ){
/*
	    RNAMotif's efn has been fixed for 0-based arrays
*/
	    for( t = 0; fgets( line, sizeof( line ), fp ); t++ ){
		sscanf( line, "%s %f", loop, &energy );
		if( t < EFN2_MAXTLOOP ){
		    ef2dp->e2_tloop[ t+1 ][ 0 ] = packloop( loop );
		    ef2dp->e2_tloop[ t+1 ][ 1 ] = NINT( 100.0 * energy );
		}
	    }
	}else
	    rval = 0;
	fclose( fp );

	if( t > EFN2_MAXTLOOP ){
	    sprintf( emsg,
"gettloop: # of tloop (%d) exceeds EFN2_MAXTLOOP (%d), last %d tloop ignored.",
		t, EFN2_MAXTLOOP, t - EFN2_MAXTLOOP );
	    RM_errormsg( FALSE, emsg );
	    ef2dp->e2_ntloops = EFN2_MAXTLOOP;
	}else
	    ef2dp->e2_ntloops = t;

	return( rval );
}

static	int	gettriloop( char fname[] )
{
	char	pname[ 256 ];
	FILE	*fp;
	char	line[ 256 ];
	char	loop[ 20 ];
	float	energy;
	int	t, rval;

	sprintf( pname, "%s/%s", rm_efndatadir, fname );
	if( ( fp = fopen( pname, "r" ) ) == NULL ){
	    sprintf( emsg, "gettriloops: can't read triloops file '%s'.",
		pname );
	    RM_errormsg( FALSE, emsg );
	    return( 0 );
	}
	rval = 1;
	if( skipto( fp, "---", sizeof( line ), line ) ){
/*
	    RNAMotif's efn has been fixed for 0-based arrays
*/
	    for( t = 0; fgets( line, sizeof( line ), fp ); t++ ){
		sscanf( line, "%s %f", loop, &energy );
		if( t < EFN2_MAXTRILOOP ){
		    ef2dp->e2_triloop[ t+1 ][ 0 ] = packloop( loop );
		    ef2dp->e2_triloop[ t+1 ][ 1 ] = NINT( 100.0 * energy );
		}
	    }
	}else
		rval = 0;
	fclose( fp );

	if( t > EFN2_MAXTRILOOP ){
	    sprintf( emsg,
"gettloops: # of triloop (%d) exceeds EFN2_MAXTRILOOP (%d), last %d triloop ignored.",
		t, EFN2_MAXTRILOOP, t - EFN2_MAXTRILOOP );
	    RM_errormsg( FALSE, emsg );
	    ef2dp->e2_ntriloops = EFN2_MAXTRILOOP;
	}else
	    ef2dp->e2_ntriloops = t;

	return( rval );
}

static	int	get1x1loop( char fname[] )
{
	char	pname[ 256 ];
	FILE	*fp;
	char	line[ 256 ];
	int	a, b, c, d, e, f;
	int	v1, v2, v3, v4, v5, v6;
	char	*fields[ 24 ];
	int	fc, n_fields;
	int	lval;
	int	rval;

	sprintf( pname, "%s/%s", rm_efndatadir, fname );
	if( ( fp = fopen( pname, "r" ) ) == NULL ){
	    sprintf( emsg, "get1x1loop: can't read 2x2 loop file '%s'",
		pname );
	    RM_errormsg( FALSE, emsg );
	    return( 0 );
	}
	rval = 1;

	/* initialize:	*/
	for( v1 = 0; v1 < N_BCODES; v1++ ){
	    for( v2 = 0; v2 < N_BCODES; v2++ ){
	    	for( v3 = 0; v3 < N_BCODES; v3++ ){
	    	    for( v4 = 0; v4 < N_BCODES; v4++ ){
	    	    	for( v5 = 0; v5 < N_BCODES; v5++ ){
	    	    	    for( v6 = 0; v6 < N_BCODES; v6++ ){
				ef2dp->e2_iloop11[v1][v2][v3][v4][v5][v6] = 0;
			    }
			}
		    }
		}
	    }
	}

	/* Skip the header */
	if( !skipto( fp, "<--", sizeof( line ), line ) ){
	    sprintf( emsg, "get2x2loop: error in int2 file '%s'.",
		pname );
	    RM_errormsg( FALSE, emsg );
	    rval = 0;
	    goto CLEAN_UP;
	}

	/* this 6 is pairs: a:u, c:g, g:c, u:a, g:u, u:g 	*/
	for( v1 = 0; v1 < 6; v1++ ){
	    if( !skipto( fp, "<--", sizeof( line ), line ) ){
			sprintf( emsg,
			"getsymint: premature end of sym-2 loop file '%s'.",
				pname );
			RM_errormsg( FALSE, emsg );
			rval = 0;
			goto CLEAN_UP;
	    }
	    a = bmap[ v1 ];
	    d = rmap[ v1 ];
	    for( b = 0; b < 4; b++ ){
		fgets( line, sizeof( line ), fp );
		n_fields = split( line, fields, " \t\n" );
		for( fc = 0; fc < n_fields; fc++ ){
		    v2 = fc / 4;
		    v4 = fc % 4;
		    c = bmap[ v2 ];
		    f = rmap[ v2 ];
		    e = v4;
		    lval = NINT( 100.0*atof( fields[ fc ] ) );
		    ef2dp->e2_iloop11[a][b][c][d][e][f] = lval;
		    free( fields[ fc ] );
		}
	    }
	}
	fclose( fp );
	fp = NULL;

CLEAN_UP : ;

	if( fp != NULL ){
	    fclose( fp );
	    fp = NULL;
	}

	return( rval );
}

static	int	get2x1loop( char fname[] )
{
	char	pname[ 256 ];
	FILE	*fp;
	char	line[ 256 ];
	int	a, b, c, d, e, f, g;
	int	v1, v2, v3, v4, v5, v6, v7;
	char	*fields[ 24 ];
	int	fc;
	int	lval;
	int	rval;

	sprintf( pname, "%s/%s", rm_efndatadir, fname );
	if( ( fp = fopen( pname, "r" ) ) == NULL ){
	    sprintf( emsg, "get2x1loop: can't read 2x1 loop file '%s'.",
		pname );
	    RM_errormsg( FALSE, emsg );
	    return( 0 );
	}
	rval = 1;

	/* initialize	*/
	for( v1 = 0; v1 < N_BCODES; v1++ ){
	    for( v2 = 0; v2 < N_BCODES; v2++ ){
		for( v3 = 0; v3 < N_BCODES; v3++ ){
		    for( v4 = 0; v4 < N_BCODES; v4++ ){
			for( v5 = 0; v5 < N_BCODES; v5++ ){
			    for( v6 = 0; v6 < N_BCODES; v6++ ){
				for( v7 = 0; v7 < N_BCODES; v7++ ){

		ef2dp->e2_iloop21[v1][v2][v3][v4][v5][v6][v7] = EFN2_INFINITY;

				}
			    }
			}
		    }
		}
	    }
	}

	/* Skip the header */
	if( !skipto( fp, "<--", sizeof( line ), line ) ){
	    sprintf( emsg, "get2x1loop: error in int21 file '%s'.",
		pname );
	    rval = 0;
	    goto CLEAN_UP;
	}

	/*  c		*/
	/* a  f		*/
	/* b  g		*/
	/*  de		*/
	/* This 6 is pairs: a:u, c:g, g:c, u:a g:u, u:g	*/
	for( v1 = 0; v1 < 6; v1++ ){
	    a = bmap[ v1 ];
	    b = rmap[ v1 ];
	    for( e = 0; e < 4; e++ ){
		if( !skipto( fp, "<--", sizeof( line ), line ) ){
		    sprintf( emsg,
			"get2x1loop: premature end of int21 file '%s'.",
			pname );
		     RM_errormsg( FALSE, emsg );
		     rval = 0;
		     goto CLEAN_UP;
		}
		for( c = 0; c < 4; c++ ){
		    fgets( line, sizeof( line ), fp );
		    split( line, fields, " \t\n" );
		    for( fc = 0, v4 = 0; v4 < 6; v4++ ){
			f = bmap[ v4 ];
			g = rmap[ v4 ];
			for( v5 = 0; v5 < 4; v5++, fc++ ){
			    d = v5;
			    lval = NINT( 100.0*atof( fields[ fc ] ) );
			    ef2dp->e2_iloop21[a][b][c][d][e][f][g] = lval;
			    free( fields[ fc ] );
			}
		    }
		}
	    }
	}

CLEAN_UP : ;
	if( fp != NULL ){
	    fclose( fp );
	    fp = NULL;
	}

	return( rval );
}

static	int	get2x2loop( char fname[] )
{
	char	pname[ 256 ];
	FILE	*fp;
	char	line[ 256 ];
	int	a, b, c, d, j, k, l, m;
	int	v1, v2, v3, v4, v5, v6, v7, v8;
	char	*fields[ 24 ];
	int	f, n_fields;
	int	lval;
	int	rval;

	sprintf( pname, "%s/%s", rm_efndatadir, fname );
	if( ( fp = fopen( pname, "r" ) ) == NULL ){
	    sprintf( emsg, "get2x2loop: can't read 2x2 loop file '%s'",
		pname );
	    RM_errormsg( FALSE, emsg );
	    return( 0 );
	}
	rval = 1;

	/* initialize:	*/
	for( v1 = 0; v1 < N_BCODES; v1++ ){
	    for( v2 = 0; v2 < N_BCODES; v2++ ){
	    	for( v3 = 0; v3 < N_BCODES; v3++ ){
	    	    for( v4 = 0; v4 < N_BCODES; v4++ ){
	    	    	for( v5 = 0; v5 < N_BCODES; v5++ ){
	    	    	    for( v6 = 0; v6 < N_BCODES; v6++ ){
	    	    	    	for( v7 = 0; v7 < N_BCODES; v7++ ){
	    	    	    	    for( v8 = 0; v8 < N_BCODES; v8++ ){

	ef2dp->e2_iloop22[v1][v2][v3][v4][v5][v6][v7][v8] = EFN2_INFINITY;

				    }
				}
			    }
			}
		    }
		}
	    }
	}

	/* Skip the header */
	if( !skipto( fp, "<--", sizeof( line ), line ) ){
	    sprintf( emsg, "get2x2loop: error in int2 file '%s'.",
		pname );
	    RM_errormsg( FALSE, emsg );
	    rval = 0;
	    goto CLEAN_UP;
	}

	/*Read the 2x2 internal loops		*/
	/*key iloop22[a][b][c][d][j][l][k][m] = */
	/*a j l b				*/
	/*c k m d				*/
	/* This 6 is pairs: a:u, c:g, g:c, u:a, g:u, u:g 	*/
	for( v1 = 0; v1 < 6; v1++ ){
	    a = bmap[ v1 ]; 
	    c = rmap[ v1 ];
	    for( v2 = 0; v2 < 6; v2++ ){
		if( !skipto( fp, "<--", sizeof( line ), line ) ){
		    sprintf( emsg,
			"getsymint: premature end of sym-2 loop file '%s'.",
			pname );
		    RM_errormsg( FALSE, emsg );
		    rval = 0;
		    goto CLEAN_UP;
		}
		b = bmap[ v2 ];
		d = rmap[ v2 ];
		for( j = 0; j < 4; j++ ){
		    for( k = 0; k < 4; k++ ){
			fgets( line, sizeof( line ), fp );
			n_fields = split(line, fields, " \t\n");
			for( f = 0; f < n_fields; f++ ){
			    l = f / 4;
			    m = f % 4;
			    lval = NINT( 100.0*atof( fields[f] ) );
			    ef2dp->e2_iloop22[a][b][c][d][j][l][k][m] = lval;
			    free( fields[ f ] );
			}
		    }
		}
	    }
	}

CLEAN_UP : ;

	if( fp != NULL ){
	    fclose( fp );
	    fp = NULL;
	}

	return( rval );
}

static	int	packloop( char loop[] )
{
	char	*lp;
	int	len, num;

	len = strlen( loop );
	for( lp = &loop[ len - 1 ], num = 0; lp >= loop; lp-- ){
		switch( *lp ){
		case 'A' :
		case 'a' :
			num = ( num * 5 ) + BCODE_A;
			break;
		case 'C' :
		case 'c' :
			num = ( num * 5 ) + BCODE_C;
			break;
		case 'G' :
		case 'g' :
			num = ( num * 5 ) + BCODE_G;
			break;
		case 'T' :
		case 't' :
		case 'U' :
		case 'u' :
			num = ( num * 5 ) + BCODE_T;
			break;
		default :
			sprintf( emsg,
				"packloop: illegal char %c (%d)", *lp, *lp );
			RM_errormsg( TRUE, emsg );
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
/*
 *	Function efn2
 *
 *	The energy calculator of Zuker calculates the free energy of each
 *	structural conformation in a structure
 *
 *	structures cannot have pseudoknots
 *
 */
int	RM_efn2( void )
{
	int	i, j, ip, jp, k;
	int	h, h1;
	int	open, null;
	int	stz, n_helix, n_upn;
	STACKSTRUCT_T	stack;
	int	energy;
	int	mw_5pgap;

	energy = 0;
	stack.sp = 0;
	push( &stack, 0, rm_l_base, 1, 0 );

SUBROUTINE : ;

	pop( &stack, &i, &j, &open, &null, &stz );

	while( stz != 1 ){

		while( rm_basepr[i] == j ){

			/* are i and j paired?	*/
			while( rm_basepr[i+1] == j-1 ){
				/* are i,j and i+1,j-1 stacked?	*/
				energy += ef2_stack(i,j,i+1,j-1);
				i++;
				j--;
			}

			n_helix = 0;
			k = i + 1;

			/* now efn2 is past the paired region, so define*/
			/* the intervening non-paired segment		*/
			while( k < j ){
				if( rm_basepr[k] > k ){
					n_helix++;
					ip = k;
					k = rm_basepr[k] + 1;
					jp = k-1;
				}else if( rm_basepr[k] == UNDEF )
					k++;
			}

			if( n_helix == 0 ){
				energy += ef2_hploop(i,j);
				goto SUBROUTINE;
			}else if( n_helix == 1 ){
				energy += ef2_ibloop(i,j,ip,jp);
				i = ip;
				j = jp;
			}else{
				n_helix++; /* include stem	*/

				if( n_helix >= MAXHELIX ){
					fprintf( stderr,
					"efn2: junction too complicated.\n" );
					return( EFN2_INFINITY );
				}

				for( h = 0; h <= n_helix; h++ )
					memset(coax[h],0,
						(n_helix+1)*sizeof(int));

		/* find each helix and store info in array helix	*/
		/* place these helixes onto the stack			*/
		/* calculate energy of the intervening unpaired nucs	*/

				helix[0][0] = i;
				helix[0][1] = j;

				n_upn = 0;
				for( h = 1; h < n_helix; h++ ){
					ip = helix[h-1][0] + 1;
					while( rm_basepr[ip] == UNDEF )
						ip++;
					/* ?add the terminal AU ef2_aupen */
					energy += ef2_aupen(ip,rm_basepr[ip]);
					helix[h][1] = ip;
					helix[h][0] = rm_basepr[ip];
					push( &stack, ip, rm_basepr[ip], 1, 0 );
					n_upn += ip - helix[h-1][0] - 1;
				}
				helix[n_helix][0] = helix[0][0];
				helix[n_helix][1] = helix[0][1];

				n_upn+=helix[n_helix][1]-helix[n_helix-1][0]-1;

				/* m-way loop bonus:			*/
				energy += ef2dp->e2_efn2a;

				/* energy for each entering helix	*/
				energy += n_helix*ef2dp->e2_efn2c;

				if( n_upn <= 6 ){
					energy += n_upn * (ef2dp->e2_efn2b);
				}else{
					energy += 6*(ef2dp->e2_efn2b)+ (int)(11.*log((double)((n_upn/6.)))+0.5);
				}

			/* Now calculate the energy of stacking:	*/
				for( h = 0; h <= n_helix; h++ ){

			/* h+1 indicates the number of helixes consider	*/
			/* h == 0: this is the energy of stacking bases	*/
			/* h == 1: is coaxial stacking better?		 */
					if( h == 0 ){
						for( h1 = 0; h1 < n_helix; h1++ ){
							coax[h1][h1] = 0;
							mw_5pgap = FALSE;
							if( h1 == 0 && helix[h1][1]-helix[n_helix-1][0] > 1 ||
								h1 != 0 && helix[h1][1]-helix[h1-1][0] > 1 )
							{
								mw_5pgap = TRUE;
							}
							if( helix[h1+1][1] - helix[h1][0] > 1 && mw_5pgap ){

					coax[h1][h1] = ef2_tstkm( helix[h1][0],helix[h1][1],helix[h1][0]+1,helix[h1][1]-1 );

							}else{
								if( helix[h1+1][1] - helix[h1][0] > 1 ){

					coax[h1][h1] = MIN( 0, ef2_dangle( helix[h1][0],helix[h1][1],helix[h1][0]+1,0 ) );

								}
								if( h1 == 0 ){
									if( helix[0][1]-helix[n_helix-1][0] > 1 ){

					coax[h1][h1] += MIN( 0, ef2_dangle( helix[h1][0],helix[h1][1],helix[h1][1]-1,1 ) );

									}
								}else{
									if( helix[h1][1]-helix[h1-1][0] > 1 ){

					coax[h1][h1] += MIN( 0, ef2_dangle( helix[h1][0],helix[h1][1],helix[h1][1]-1,1 ) );

									}
								}
							}
						}

					coax[n_helix][n_helix] = coax[0][0];

					}else if( h == 1 ){
						for( h1 = 0; h1 < n_helix; h1++ ){
							if( helix[h1+1][1] - helix[h1][0] == 1 ){

					coax[h1][h1+1] = MIN( coax[h1][h1] + coax[h1+1][h1+1],
						ef2_coax( helix[h1][1],helix[h1][0],helix[h1+1][1],helix[h1+1][0] ) );

							}else if( helix[h1+1][1] - helix[h1][0] == 2 ){
								coax[h1][h1+1] = coax[h1][h1]+coax[h1+1][h1+1];
								if( h1 != 0 ){
									if( helix[h1][1] - helix[h1-1][0] > 1 ){

					coax[h1][h1+1] = MIN( coax[h1][h1+1],
						ef2_tstackcoax( helix[h1][0],  helix[h1][1],  helix[h1][0]+1,helix[h1][1]-1 ) +
						ef2_coaxstack ( helix[h1][0]+1,helix[h1][1]-1,helix[h1+1][1],helix[h1+1][0] ) );

									}
								}else{
									if( helix[0][1]-helix[n_helix-1][0] > 1 ){

					coax[h1][h1+1] = MIN( coax[h1][h1+1],
						ef2_tstackcoax( helix[h1][1],  helix[h1][0],  helix[h1][0]+1,helix[h1][1]-1 ) +
						ef2_coaxstack ( helix[h1][0]+1,helix[h1][1]-1,helix[h1+1][1],helix[h1+1][0] ) );

									}
								}
								if( h1 != n_helix-1 ){
									if( helix[h1+2][1]-helix[h1+1][0] > 1 ){

					coax[h1][h1+1] = MIN( coax[h1][h1+1],
						ef2_tstackcoax( helix[h1][0]+1,helix[h1+1][0]+1,helix[h1+1][1],helix[h1+1][0] ) +
						ef2_coaxstack ( helix[h1][0],  helix[h1][1],    helix[h1][0]+1,helix[h1+1][0]+1 ) );

									}
								}else{
									if( helix[1][1]-helix[0][0] > 1 ){

					coax[h1][h1+1] = MIN( coax[h1][h1+1],
						ef2_tstackcoax( helix[h1][0]+1,helix[h1+1][0]+1,helix[h1+1][1],helix[h1+1][0] ) +
						ef2_coaxstack ( helix[h1][0],  helix[h1][1],    helix[h1][0]+1,helix[h1+1][0]+1 ) );

									}
								}
							}else{

					coax[h1][h1+1] = coax[h1][h1] + coax[h1+1][h1+1];

							}
						}
					}else if( h > 1 && h < n_helix ){
						for( i=0; i+h <= n_helix; i++ ){
							coax[i][i+h] = coax[i][i]+coax[i+1][i+h];
							for( j=1; j<h; j++ ){
								coax[i][i+h] = MIN( coax[i][i+h], coax[i][i+j]+coax[i+j+1][i+h] );
							}
						}
					}else if( h == n_helix ){
						energy += MIN( coax[0][n_helix-1], coax[1][n_helix] );
					}
				}

				goto SUBROUTINE;
			}
		}

	/* this is the exterior loop: i = 1				*/
	/* Find number of helixes exiting the loop, store in n_helix:	*/
		n_helix = 0;
		while( i < rm_l_base ){	 	/* Fix me!	*/
			if( rm_basepr[i] != UNDEF ){
				n_helix++;
				i = rm_basepr[i];
			}
			i++;
		}

		if( n_helix >= MAXHELIX ){
			fprintf( stderr, "efn2: junction too complicated.\n" );
			return( EFN2_INFINITY );
		}

		for( h = 0; h < n_helix; h++ ){
			memset( coax[h], 0, n_helix * sizeof( int ) );
		}

		/* find each helix and store info in array helix	*/
		/* also place these helixes onto the stack		*/
		ip = 1;
		for( h = 0; h < n_helix; h++ ){
			while( rm_basepr[ip]==0 )
				ip++;
			/* ?add the terminal AU ef2_aupen */
			energy += ef2_aupen(ip,rm_basepr[ip]);
			helix[h][1] = ip;
			helix[h][0] = rm_basepr[ip];
			push( &stack, ip, rm_basepr[ip], 1, 0 );
			ip = rm_basepr[ip]+1;
		}

		/* Now calculate the energy of stacking:	*/
		for( h = 0; h < n_helix; h++ ){
			/* h+1 indicates the number of helixes consider	*/
			/* h == 0: this is the energy of stacking bases	*/
			/* h == 1: is coaxial stacking better?		*/
			if( h == 0 ){
				for( h1 = 0; h1 < n_helix; h1++ ){
					coax[h1][h1] = 0;
					if( h1 < n_helix-1 ){
			/* not at 3' end of structure	*/
						if( helix[h1+1][1] - helix[h1][0] > 1 ){

			/* try 3' dangle	*/
			coax[h1][h1] = MIN( 0, ef2_dangle( helix[h1][0],helix[h1][1],helix[h1][0]+1,0 ) );

						}
					}else{
						/* at 3' end of structure	*/
						if( rm_l_base - helix[h1][0] >= 1 ){	/* fix me! */

			/* try 3' dangle	*/
			coax[h1][h1] = MIN( 0, ef2_dangle( helix[h1][0],helix[h1][1], helix[h1][0]+1,0 ) );

						}
					}
					if( h1 == 0 ){
						if( helix[0][1] > 1 ){

			coax[h1][h1] += MIN( 0, ef2_dangle( helix[h1][0],helix[h1][1], helix[h1][1]-1,1 ) );

						}
					}else{
						if( helix[h1][1]-helix[h1-1][0] >= 1 ){

			coax[h1][h1] += MIN( 0, ef2_dangle( helix[h1][0],helix[h1][1], helix[h1][1]-1,1 ) );

						}
					}
				}
			}else if( h == 1 ){
				for( h1 = 0; h1 < n_helix-1; h1++ ){
					/* see if they're close enough to stack	*/
					if( helix[h1+1][1] - helix[h1][0] == 1 ){ /* flush stacking:	*/

			coax[h1][h1+1] = MIN( coax[h1][h1] + coax[h1+1][h1+1],
				ef2_coax( helix[h1][1],helix[h1][0],helix[h1+1][1],helix[h1+1][0] ) );

					}else if( helix[h1+1][1] - helix[h1][0] == 2 ){ /* possible 1 ss nt	*/
						coax[h1][h1+1] = coax[h1][h1]+coax[h1+1][h1+1];
						if( h1 != 0 ){
							if( helix[h1][1] - helix[h1-1][0] > 1 ){

			coax[h1][h1+1] = MIN( coax[h1][h1+1],
				ef2_tstackcoax( helix[h1][0],  helix[h1][1],  helix[h1][0]+1,helix[h1][1]-1 ) +
				ef2_coaxstack ( helix[h1][0]+1,helix[h1][1]-1,helix[h1+1][1],helix[h1+1][0] ) );

							}
						}else{
							if( helix[0][1] > 1 ){

			coax[h1][h1+1] = MIN( coax[h1][h1+1],
				ef2_tstackcoax( helix[h1][1],  helix[h1][0],  helix[h1][0]+1,helix[h1][1]-1 ) +
				ef2_coaxstack ( helix[h1][0]+1,helix[h1][1]-1,helix[h1+1][1],helix[h1+1][0] ) );

							}
						}
						if( h1 != n_helix-2 ){
							if( helix[h1+2][1]-helix[h1+1][0] > 1 ){

			coax[h1][h1+1] = MIN( coax[h1][h1+1],
				ef2_tstackcoax( helix[h1][0]+1,helix[h1+1][0]+1,helix[h1+1][1],helix[h1+1][0] ) +
				ef2_coaxstack ( helix[h1][0],  helix[h1][1],    helix[h1][0]+1,helix[h1+1][0]+1 ) );

							}
						}else{
							if( helix[n_helix-1][0]<rm_l_base ){	/* fix me */

			coax[h1][h1+1] = MIN( coax[h1][h1+1],
				ef2_tstackcoax( helix[h1][0]+1,helix[h1+1][0]+1,helix[h1+1][1],helix[h1+1][0] ) +
				ef2_coaxstack ( helix[h1][0],  helix[h1][1],    helix[h1][0]+1,helix[h1+1][0]+1 ) );

							}
						}
					}else{ /* no possible stacks	*/
						coax[h1][h1+1] = coax[h1][h1] + coax[h1+1][h1+1];
					}
				}
			}else if( h > 1 ){
				for( i = 0; i+h < n_helix; i++ ){
					coax[i][i+h] = coax[i][i]+coax[i+1][i+h];
					for( j = 1; j < h; j++ ){
						coax[i][i+h] = MIN( coax[i][i+h], coax[i][i+j]+coax[i+j+1][i+h] );
					}
				}
			}
			if( h == n_helix-1 )
				energy += coax[0][n_helix-1];
/*
			}else if( h == n_helix-1 )
				energy += coax[0][n_helix-1];
*/
		}

		goto SUBROUTINE;

		/*
		 *	The original main loop of efn()
		 *
		 *	// whittle away at 5' end:
		 *	while( ct->basepr[count][i]==0 &&
		 *			ct->basepr[count][i+1]==0 )
		 *	{
		 *		i++;
		 *		if (i>=(j-1)) goto SUBROUTINE;
		 *	}
		 *	//whittle away at 3' end
		 *	while( ct->basepr[count][j]==0 &&
		 *		ct->basepr[count][j-1]==0 )
		 *	{
		 *		j--;
		 *		 if (i>=j-1) goto SUBROUTINE;
		 *	}
		 *	//i dangles on i+1
		 *	if( ct->basepr[count][i]==0 &&
		 *		ct->basepr[count][i+1]>(i+1))
		 *	{
		 *		ct->energy[count] = ct->energy[count]+
		 *			min(0,ef2_dangle(ct->basepr[count][i+1],
		 *			(i+1),i,2,ct,data));
		 *		i++;
		 *	}
		 *	//j dangles on j-1
		 *	if ((ct->basepr[count][j]==0&&
		 *		ct->basepr[count][j-1]!=0)&&
		 *		ct->basepr[count][j-1]<(j-1))
		 *	{
		 *		ct->energy[count]=ct->energy[count] +
		 *			min(0,ef2_dangle(j-1,
		 *			ct->basepr[count][j-1],j,1,ct,data));
		 *		j--;
		 *	}
		 *	//structure bifurcates
		 *	if (ct->basepr[count][i]!=j ){
		 *		k = ct->basepr[count][i];
		 *		if( ct->basepr[count][k+1]!=0 ){
		 *			push (&stack,i,k,open,0);
		 *			push (&stack,k+1,j,open,0);
		 *		}else if( ct->basepr[count][k+2]==0 ){
		 *			push (&stack,i,k+1,open,0);
		 *			push (&stack,k+2,j,open,0);
		 *		}else if( ef2_dangle(k,i,k+1,1,ct,data)<=
		 *			ef2_dangle((ct->basepr[count][k+2]),
		 *			k+2,k+1,2,ct,data))
		 *		{
		 *			push (&stack,i,k+1,open,0);
		 *				push (&stack,k+2,j,open,0);
		 *		} else {
		 *			push (&stack,i,k,open,0);
		 *			push (&stack,k+1,j,open,0);
		 *		}
		 *		goto SUBROUTINE;
		 *	}
		 *	push (&stack,i,j,open,null);
		 *	goto SUBROUTINE;
		 */
	}

	return( energy );
}

static	void	push( STACKSTRUCT_T *stack, int a, int b, int c, int d )
{

	( stack->sp )++;
	stack->stk[stack->sp][0] = a;
	stack->stk[stack->sp][1] = b;
	stack->stk[stack->sp][2] = c;
	stack->stk[stack->sp][3] = d;
}

static	void	pop( STACKSTRUCT_T *stack,
	int *i, int *j, int *open, int *null, int *stz )
{
	if( stack->sp == 0 ){
		*stz = 1;
		return;
	}else{
		*stz = 0;
		*i = stack->stk[stack->sp][0];
		*j = stack->stk[stack->sp][1];
		*open = stack->stk[stack->sp][2];
		*null = stack->stk[stack->sp][3];
		stack->sp--;
	}
}

/* calculate the energy of stacked base pairs */
static	int	ef2_stack( int i, int j, int ip, int jp )
{
	int	energy;

	energy = ef2dp->e2_stack[(rm_bcseq[i])][(rm_bcseq[j])]
		[(rm_bcseq[ip])][(rm_bcseq[jp])]+ef2dp->e2_eparam[1];
	return( energy );
}

static	int ef2_ibloop( int i, int j, int ip, int jp )
{
	int	energy, size, size1, size2;
	int	loginc, lopsid;

	/* i is paired to j; ip is paired to jp; ip > i; j > jp	*/

	size1 = ip - i - 1;
	size2 = j- jp - 1;
	if( size1 == 0 || size2 == 0 ){
		size = size1 + size2;
		if( size == 1 ){
			energy = ef2dp->e2_stack[rm_bcseq[i]][rm_bcseq[j]]
				[rm_bcseq[ip]][rm_bcseq[jp]]
				+ ef2dp->e2_bulge[size] + ef2dp->e2_eparam[2];
		}else if( size > 30 ){
			loginc = ( int )((ef2dp->e2_prelog) *
					log(((double)size)/30.0));
				energy = ef2dp->e2_bulge[30] + loginc +
					ef2dp->e2_eparam[2];
				energy += ef2_aupen(i,j) + ef2_aupen(jp,ip);
		}else{
			energy = ef2dp->e2_bulge[size] + ef2dp->e2_eparam[2];
			energy += ef2_aupen(i,j) + ef2_aupen(jp,ip);
		}
	}else{
		size = size1 + size2;
		lopsid = abs(size1-size2);
		if( size > 30 ){
			loginc = ( int )((ef2dp->e2_prelog) *
				log(((double)size)/30.0));
			if( ( size1==1 || size2==1 ) && ef2dp->e2_gail ){

	energy =
		ef2dp->e2_tstki[rm_bcseq[i ]][rm_bcseq[j ]][1][1] +
		ef2dp->e2_tstki[rm_bcseq[jp]][rm_bcseq[ip]][1][1] +
		ef2dp->e2_inter[30] + loginc + ef2dp->e2_eparam[3] +
		MIN(ef2dp->e2_maxpen,lopsid* ef2dp->e2_poppen[MIN(2,MIN(size1,size2))]);

			}else{

	energy =
		ef2dp->e2_tstki[rm_bcseq[i ]][rm_bcseq[j ]][rm_bcseq[i+1 ]][rm_bcseq[j-1 ]] +
		ef2dp->e2_tstki[rm_bcseq[jp]][rm_bcseq[ip]][rm_bcseq[jp+1]][rm_bcseq[ip-1]] +
		ef2dp->e2_inter[30] + loginc + ef2dp->e2_eparam[3] +
		MIN(ef2dp->e2_maxpen,lopsid* ef2dp->e2_poppen[MIN(2,MIN(size1,size2))]);

			}
		}else if( size1 == 2 && size2 == 2 ){

	energy = ef2dp->e2_iloop22[rm_bcseq[i]][rm_bcseq[ip]] [rm_bcseq[j]][rm_bcseq[jp]]
		[rm_bcseq[i+1]][rm_bcseq[i+2]] [rm_bcseq[j-1]][rm_bcseq[j-2]];

		}else if( size1 == 1 && size2 == 2 ){

	energy = ef2dp->e2_iloop21[rm_bcseq[i]][rm_bcseq[j]][rm_bcseq[i+1]]
		[rm_bcseq[j-1]][rm_bcseq[jp+1]][rm_bcseq[ip]][rm_bcseq[jp]];

		}else if( size1 == 2 && size2 == 1 ) {

	energy = ef2dp->e2_iloop21[rm_bcseq[jp]][rm_bcseq[ip]][rm_bcseq[jp+1]]
		[rm_bcseq[ip-1]][rm_bcseq[i+1]][rm_bcseq[j]][rm_bcseq[i]];

		}else if( size == 2 ){

	energy = ef2dp->e2_iloop11[rm_bcseq[i]][rm_bcseq[i+1]][rm_bcseq[ip]]
		[rm_bcseq[j]][rm_bcseq[j-1]][rm_bcseq[jp]];

		}else if( ( size1 == 1 || size2 == 1 ) && ef2dp->e2_gail ){

	energy =
		ef2dp->e2_tstki[rm_bcseq[i ]][rm_bcseq[j ]][1][1] +
		ef2dp->e2_tstki[rm_bcseq[jp]][rm_bcseq[ip]][1][1] +
		ef2dp->e2_inter[size] + ef2dp->e2_eparam[3] +
		MIN(ef2dp->e2_maxpen, lopsid* ef2dp->e2_poppen[MIN(2,MIN(size1,size2))]);

		}else{

	energy =
		ef2dp->e2_tstki[rm_bcseq[i ]][rm_bcseq[j ]][rm_bcseq[i+1 ]][rm_bcseq[j-1 ]] +
		ef2dp->e2_tstki[rm_bcseq[jp]][rm_bcseq[ip]][rm_bcseq[jp+1]][rm_bcseq[ip-1]] +
		ef2dp->e2_inter[size] + ef2dp->e2_eparam[3] +
		MIN(ef2dp->e2_maxpen,lopsid*ef2dp->e2_poppen[MIN(2,MIN(size1,size2))]);
		}
	}

	return( energy );
}

static	int	ef2_hploop( int i, int j )
{
	int	energy, size, loginc, tlink, count, key, k;

	size = j - i - 1;
	if( size > 30 ){
		loginc = ( int )((ef2dp->e2_prelog)*log(((double)size)/30.0));
		energy = ef2dp->e2_tstkh[rm_bcseq[i]][rm_bcseq[j]]
			[rm_bcseq[i+1]][rm_bcseq[j-1]]
			+ ef2dp->e2_hairpin[30]+loginc+ef2dp->e2_eparam[4];
	}else if( size < 3 ){
		energy = ef2dp->e2_hairpin[size] + ef2dp->e2_eparam[4];
		if( rm_bcseq[i]==4 || rm_bcseq[j]==4 )
			energy = energy+6;
	}else if( size == 4 ){
		tlink = 0;
		key = (rm_bcseq[j])*3125 + (rm_bcseq[i+4])*625 +
			(rm_bcseq[i+3])*125 + (rm_bcseq[i+2])*25 +
			(rm_bcseq[i+1])*5+(rm_bcseq[i]);
		for( count=1; count<=ef2dp->e2_ntloops&&tlink==0; count++ ){
			if (key==ef2dp->e2_tloop[count][0])
				tlink = ef2dp->e2_tloop[count][1];
		}
		energy = ef2dp->e2_tstkh[rm_bcseq[i]][rm_bcseq[j]]
			[rm_bcseq[i+1]][rm_bcseq[j-1]]
			+ ef2dp->e2_hairpin[size] + ef2dp->e2_eparam[4] + tlink;
	}else if( size == 3 ){
		tlink = 0;
		key = (rm_bcseq[j])*625 +
			(rm_bcseq[i+3])*125 + (rm_bcseq[i+2])*25 +
			(rm_bcseq[i+1])*5+(rm_bcseq[i]);
		for( count=1; count<=ef2dp->e2_ntriloops&&tlink==0; count++ ){
			if (key==ef2dp->e2_triloop[count][0])
				tlink = ef2dp->e2_triloop[count][1];
		}
		energy = ef2dp->e2_tstkh[rm_bcseq[i]][rm_bcseq[j]]
			[rm_bcseq[i+1]][rm_bcseq[j-1]];
		energy = ef2dp->e2_hairpin[size] + ef2dp->e2_eparam[4] +
			tlink +ef2_aupen(i,j);
	}else{
		energy = ef2dp->e2_tstkh[rm_bcseq[i]][rm_bcseq[j]]
			[rm_bcseq[i+1]][rm_bcseq[j-1]] +
			ef2dp->e2_hairpin[size] + ef2dp->e2_eparam[4];
	}

	/* check for GU closeure preceded by GG	*/
	if( rm_bcseq[i]==BCODE_G&&rm_bcseq[j]==BCODE_T ){
		if( i > 1 && i < rm_l_base ){
			if( rm_bcseq[i-1]==BCODE_G&&rm_bcseq[i-2]==BCODE_G ){
				energy += ef2dp->e2_gubonus;
				/* if (rm_bcseq[i+1]==4&&rm_bcseq[j-1]==4)*/
				/*	 energy = energy - ef2dp->uubonus;*/
				/* if (rm_bcseq[i+1]==3&&rm_bcseq[j-1]==1)*/
				/*	 energy = energy - ef2dp->uubonus;*/
			}
		}
	}

	/* check for a poly-c loop	*/
	tlink = 1;
	for( k = 1; k <= size && tlink == 1; k++ ){
		if( rm_bcseq[i+k] != BCODE_C )
			tlink = 0;
	}
	if( tlink == 1 ){
		/* this is a poly c loop so penalize	*/
		if( size == 3 )
			energy += ef2dp->e2_c3;
		else
			energy += ef2dp->e2_cint + size*ef2dp->e2_cslope;
	}
	return( energy );
}

static	int	ef2_dangle( int i, int j, int ip, int jp )
{
	int	energy;

	/* dangling base	*/
	/* jp = 1 => 3' dangle	*/
	/* jp = 2 => 5' dangle	*/

	energy = ef2dp->e2_dangle[rm_bcseq[i]][rm_bcseq[j]][rm_bcseq[ip]][jp];
	return( energy );
}

static	int	ef2_aupen( int i, int j )
{

	/* is terminal i,j an AU?	*/
	if( rm_bcseq[i]==BCODE_T || rm_bcseq[j]==BCODE_T )
		return( ef2dp->e2_auend );
	else
		return( 0 );
}

static	int	ef2_tstkm( int i, int j, int ip, int jp )
{
	int	energy;

	energy = ef2dp->e2_tstkm[rm_bcseq[i]][rm_bcseq[j]][rm_bcseq[ip]][rm_bcseq[jp]];
	return( energy );
}

static	int	ef2_coax( int i, int j, int ip, int jp )
{
	int	energy;

/*
	ef2dp->e2_coax[rm_bcseq[helix[h1][1]]][rm_bcseq[helix[h1][0]]][rm_bcseq[helix[h1+1][1]]][rm_bcseq[helix[h1+1][0]]] );
*/
	energy = ef2dp->e2_coax[rm_bcseq[i]][rm_bcseq[j]][rm_bcseq[ip]][rm_bcseq[jp]];
	return( energy );
}

static	int	ef2_tstackcoax( int i, int j, int ip, int jp )
{
	int	energy;

/*
	ef2dp->e2_tstackcoax[rm_bcseq[helix[h1][0]]]  [rm_bcseq[helix[h1][1]]]  [rm_bcseq[helix[h1][0]+1]][rm_bcseq[helix[h1][1]-1]] +
*/
	energy = ef2dp->e2_tstackcoax[rm_bcseq[i]][rm_bcseq[j]][rm_bcseq[ip]][rm_bcseq[jp]];
	return( energy );
}

static	int	ef2_coaxstack( int i, int j, int ip, int jp )
{
	int	energy;

/*
	ef2dp->e2_coaxstack [rm_bcseq[helix[h1][0]+1]][rm_bcseq[helix[h1][1]-1]][rm_bcseq[helix[h1+1][1]]][rm_bcseq[helix[h1+1][0]]] );
*/
	energy = ef2dp->e2_coaxstack[rm_bcseq[i]][rm_bcseq[j]][rm_bcseq[ip]][rm_bcseq[jp]];
	return( energy );
}
