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
#include <stdlib.h>

#include "rmdefs.h"
#include "rnamot.h"

int	rm_error;
char	*rm_wdfname;
int	rm_emsg_lineno = UNDEF;

char	rm_bc2b[ N_BCODES ] = { 'a', 'c', 'g', 't', 'n' };

char	rm_efndatadir[ 256 ];
int	rm_efnds_allocated;
int	rm_efndataok;
int	rm_l_base;
int	*rm_hstnum;
int	*rm_bcseq;
int	*rm_basepr;

char	*getenv();

static	int	getct( FILE * );
static	int	base2num( int );

int	RM_getefndata( void );
int	RM_knotted( void );
int	RM_efn( int, int, int );

main( argc, argv )
int	argc;
char	*argv[];
{
	FILE	*cfp = NULL;
	char	*ep;
	int	n_bases;
	int	e;
	int	rval = 0;

	if( ( ep = getenv( "EFNDATA" ) ) == NULL ){
		fprintf( stderr, "%s: EFNDATA not defined.\n", argv[ 0 ] );
		rval = 1;
		goto CLEAN_UP;
	}else{
		strcpy( rm_efndatadir, ep );
		if( ( rm_efndataok = RM_getefndata() ) == 0 ){
			rm_efndataok = FALSE; 
			rval = 1;
			goto CLEAN_UP;
		}
	}

	if( argc == 1 ){
		cfp = stdin;
		rm_wdfname = " -- stdin -- ";
	}else if( argc > 2 ){
		fprintf( stderr, "usage: %s [ ct-file ]\n", argv[ 0 ] );
		rval = 1;
		goto CLEAN_UP;
	}else if( ( cfp = fopen( argv[ 1 ], "r" ) ) == NULL ){
		fprintf( stderr, "%s: can't read ct-file %s\n",
			argv[ 0 ], argv[ 1 ] );
		rval = 1;
		goto CLEAN_UP;
	}else
		rm_wdfname = argv[ 1 ];

	if( ( n_bases = getct( cfp ) ) == 0 ){
		rval = 1;
		goto CLEAN_UP;
	}
	rm_l_base = n_bases - 1;

	/* check for knots, abort if found.	*/
	if( RM_knotted() ){
		rval = 1;
		goto CLEAN_UP;
	}

	RM_initst();
	e = RM_efn( 0, rm_l_base, TRUE );
	printf( "energy = %8.3f\n", 0.01 * e );

CLEAN_UP : ;
	if( cfp != NULL && cfp != stdin )
		fclose( cfp );

	exit( rval );
}

static	int	getct( FILE *fp )
{
	char	line[ 256 ];
	int	i, npr;
	int	bn, prev, next, pn, hn;
	char	b[ 2 ];

	fgets( line, sizeof( line ), fp );
	sscanf( line, "%d", &npr );

	if( RM_allocefnds( npr ) )
		return( 0 );

	for( i = 0; i < npr; i++ ){
		fgets( line, sizeof( line ), fp );
		sscanf( line, "%d %s %d %d %d %d",
			&bn, b, &prev, &next, &pn, &hn );
		bn--;
		pn = pn == 0 ? UNDEF : pn - 1;
		rm_bcseq[ bn ] = base2num( *b );
		rm_basepr[ bn ] = pn;
		rm_hstnum[ bn ] = hn;
	}

	return( npr );
}

static	int	base2num( int b )
{

	switch( b ){
	case 'a' :
	case 'A' :
		return( BCODE_A );
		break;
	case 'c' :
	case 'C' :
		return( BCODE_C );
		break;
	case 'g' :
	case 'G' :
		return( BCODE_G );
		break;
	case 't' :
	case 'T' :
	case 'u' :
	case 'U' :
		return( BCODE_T );
		break;

	default :
		return( BCODE_N );
		break;
	}
}
