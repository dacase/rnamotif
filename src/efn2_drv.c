#include <stdio.h>
#include <string.h>

#include "rmdefs.h"
#include "rnamot.h"

int	rm_error;
char	*rm_wdfname;
int	rm_emsg_lineno;

#define	SIZE	6000
char	rm_efndatadir[ 256 ] = "";
int	rm_l_base;
int	*rm_hstnum;
int	*rm_bcseq;
int	*rm_basepr;

int	efn2_nbases;
char	efn2_header[ 256 ];
char	*efn2_nucs;
int	efn2_energy;

char	*getenv();

void	CT_write( FILE * );
static	int	CT_b2i( int );

main( int argc, char* argv[] )
{
	FILE		*cfp;
	char		*dhp;
	int		n_struct;
	int		rval = 0;

	if( ( dhp = getenv( "EFNDATA" ) ) == NULL ){
		fprintf( stderr, "%s: EFNDATA not defined.\n", argv[ 0 ] );
		rval = 1;
		goto CLEAN_UP;
	}else
		strcpy( rm_efndatadir, dhp );

	if( argc == 1 ){
		cfp = stdin;
		rm_wdfname = " --stdin-- ";
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

	rm_hstnum = ( int * )malloc( SIZE * sizeof( int ) );
	rm_bcseq = ( int * )malloc( SIZE * sizeof( int ) );
	rm_basepr = ( int * )malloc( SIZE * sizeof( int ) );
	efn2_nucs = ( char * )malloc( SIZE * sizeof( char ) );

	if( !RM_getefn2data() ) {
		fprintf( stderr, "%s: opendat errors.\n", argv[ 0 ] );
		rval = 1;
	}

	for( n_struct = 0; ( efn2_nbases = CT_read( cfp ) ) != 0; n_struct++ ){
		rm_l_base = efn2_nbases - 1;
		efn2_energy = RM_efn2();
		CT_write( stdout );
	}

	if( cfp != NULL && cfp != stdin ){
		fclose( cfp );
		cfp = NULL;
	}

CLEAN_UP : ;

	exit( rval );
}

void errmsg( int err,int erri )
{

	switch( err ){
	case '1':
		fprintf( stderr,
			"errmsg: Could not allocate enough memory.\n" );
		break;
	case '2':
		fprintf( stderr, "errmsg: Too many possible base pairs.\n" );
		break;
	case '3':
		fprintf( stderr,
			"errmsg: Too many helixes in multibranch loop\n" );
		break;
	case '4':
		fprintf( stderr, "errmsg: Too many structures in CT file.\n" );
		break;
	case 30:
		fprintf( stderr, "errmsg: End reached at traceback: %d\n",
			erri );
		break;
	case 100:
		fprintf( stderr, "errmsg: error: %d\n", erri );
		break;
	default:
		fprintf( stderr, "errmsg: Unknown error.\n" );
		break;
	}

	return;
}

int	CT_read( FILE *cfp )
{
	char	line[ 256 ];
	int	nbases, i;
	char	*hp;

	if( !fgets( line, sizeof( line ), cfp ) )
		return( 0 );

	sscanf( line, "%d %[^\n]", &nbases, efn2_header );
	for( hp = efn2_header; *hp; hp++ )
		;
	for( --hp; hp >= efn2_header && isspace( *hp ); hp-- ) 
		;
	if( !isspace( *hp ) )
		hp++;
	*hp = '\0';

	for( i = 0; i < nbases; i++ ){
		fgets( line, sizeof( line ), cfp );
		sscanf( line, "%*d %c %*d %*d %d %d",
			&efn2_nucs[i], &rm_basepr[i], &rm_hstnum[i] );
		if( rm_basepr[i] != 0 )
			rm_basepr[i]--;
		else
			rm_basepr[i] = UNDEF;
		rm_bcseq[i] = CT_b2i( efn2_nucs[i] );
	}

CLEAN_UP : ;

	return( nbases );
}

void	CT_write( FILE *cfp )
{
	int	i;

	fprintf( cfp, "%5d", efn2_nbases );
	fprintf( cfp, "   dG = %5.2f",  0.01*efn2_energy );
	fprintf( cfp, " %-.40s\n", efn2_header );
	for( i = 0; i < efn2_nbases; i++ ){
		fprintf( cfp, "%4d %c %4d %4d %4d %4d\n",
			i+1, efn2_nucs[ i ],
			i, ( i < efn2_nbases-1 ) ? i+2 : 0,
			( rm_basepr[ i ] == UNDEF ) ? 0 : rm_basepr[i]+1,
			rm_hstnum[ i ] );
	}
}

static	int	CT_b2i( int b )
{

	switch( b ){
	case 'A' :
	case 'a' :
		return( BCODE_A );
		break;
	case 'C' :
	case 'c' :
		return( BCODE_C );
		break;
	case 'G' :
	case 'g' :
		return( BCODE_G );
		break;
	case 'T' :
	case 't' :
	case 'U' :
	case 'u' :
		return( BCODE_T );
		break;
	default :
		return( BCODE_N );
		break;
	}
}
