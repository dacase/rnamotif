#include <stdio.h>
#include <ctype.h>
#include <string.h>

#define	UNDEF	(-1)

#include "genbank.h"

#include "dbase.h"

#define	FMAP_SIZE	100
static	GB_FMAP_T	fmap[ FMAP_SIZE ];
static	int		n_fmap;

static	char	s_dbname[ 256 ];
static	char	*s_dbnp;

static	GB_HIT_T	*ihits;
static	int	n_ihits;

static	int	*iref;
static	int	n_iref;

#define	EBUF_SIZE	4000000
static	char	ebuf[ EBUF_SIZE ];
static	GB_ENTRY_T	gentry;
#define	GBREFS_SIZE	1000
static	GB_REFERENCE_T	gbrefs[ GBREFS_SIZE ];
#define	GBFTAB_SIZE	5000
static	GB_FEATURE_T	gbftab[ GBFTAB_SIZE ];
#define	GBQTAB_SIZE	10000
static	GB_QUALIFIER_T	gbqtab[ GBQTAB_SIZE ];
 
DBASE_T	*DB_open( dbname )
char	dbname[];
{
	GB_FMAP_T	*fmp;
	int	i;
	int	first, last, swap, scnt;
	DBASE_T	*dbp;
	FILE	*ifp;
	GB_HEADER_T	ihdr;

	if( n_fmap == 0 ){
		n_fmap = GB_read_fmap( NULL, FMAP_SIZE, fmap );
		if( n_fmap == 0 ){
			fprintf( stderr, "DB_open: can't read file map.\n" );
			return( NULL );
		}
	}

	first = last = UNDEF;
	for( fmp = fmap, i = 0; i < n_fmap; i++, fmp++ ){
		if( !strcmp( fmp->f_dbname, dbname ) ){
			if( first == UNDEF )
				first = fmp->f_dbfirst;
			last = fmp->f_dblast;
		}
	}

	dbp = NULL;
	if( first != UNDEF ){
		dbp = ( DBASE_T * )malloc( sizeof( DBASE_T ) );
		if( dbp == NULL ){
			fprintf( stderr,
			"DB_open: can't allocate dbp for STD dbase.\n" );
			return( NULL );
		}
		dbp->d_type = DB_STD;
		dbp->d_first = first;
		dbp->d_last = last;
		dbp->d_current = first;
		dbp->d_iref = NULL;
		return( dbp );
	}else if( ( ifp = fopen( dbname, "r" ) ) == NULL ){
		fprintf( stderr, "DB_open: can't read database '%s'.\n",
			dbname );
		return( NULL );
	}

	if( GB_read_hdr2( ifp, &ihdr, &swap ) ){
		fprintf( stderr,
			"DB_open: incomplete header in iref/ihits file.\n" );
		goto CLEAN_UP;
	}

	GB_print_hdr( stderr, &ihdr );

	if( GB_IS_HDR( ihdr.h_ftype, GB_FT_HITS ) ){
		n_ihits = ihdr.h_count;
		ihits = ( GB_HIT_T * )malloc( n_ihits * sizeof( GB_HIT_T ) );
		if( ihits == NULL ){
			fprintf( stderr,
				"DB_open: can't allocate ihits.\n" );
			goto CLEAN_UP;
		}
		if(fread(ihits,sizeof(GB_HIT_T),(long)n_ihits,ifp) != n_ihits){
			fprintf( stderr, "DB_open: error in ihits file.\n" );
			goto CLEAN_UP;
		}else if( swap ){
			scnt = sizeof( GB_HIT_T ) / sizeof( int ) * n_ihits;
			GB_swap4bytes( ihits, scnt );
		}
		for( i = 0; i < n_ihits; i++ ){
			ihits[ i ].h_info = 0;
			ihits[ i ].h_aux = 0;
			ihits[ i ].h_mark = 0;
		}
		n_iref = GB_hitcompress( n_ihits, ihits );
		iref = ( int * )malloc( n_iref * sizeof( int ) );
		if( iref == NULL ){
			fprintf( stderr,
				"DB_open: can't allocate iref for ihits.\n" );
			goto CLEAN_UP;
		}
		for( i = 0; i < n_iref; i++ )
			iref[ i ] = ihits[ i ].h_iref;
	}else if( GB_IS_HDR( ihdr.h_ftype, GB_FT_IREF ) ){
		n_iref = ihdr.h_count;
		iref = ( int * )malloc( n_iref * sizeof( int ) );
		if( iref == NULL ){
			fprintf( stderr,
				"DB_open: can't allocate iref.\n" );
			goto CLEAN_UP;
		}
		if( fread(iref, sizeof(int), (long)n_iref, ifp ) != n_iref ){
			fprintf( stderr, "DB_open: error in iref file.\n" );
			goto CLEAN_UP;
		}else if( swap )
			GB_swap4bytes( iref, n_iref );
	}else{
		fprintf( stderr,
			"DB_open: iref/ihits file type %.*s.\n",
			GB_FTYPE_SIZE, ihdr.h_ftype );
		goto CLEAN_UP;
	}

	dbp = ( DBASE_T * )malloc( sizeof( DBASE_T ) );
	if( dbp == NULL ){
		fprintf( stderr,
			"DB_open: can't allocate dbp for USER dbase.\n" );
		goto CLEAN_UP;
	}
	dbp->d_type = DB_USER;
	dbp->d_first = 0;
	dbp->d_last = n_iref;
	dbp->d_current = 0;
	dbp->d_iref = iref;

CLEAN_UP : ;

	fclose( ifp );
	return( dbp );
}

DBASE_T	*DB_next( dbp )
DBASE_T	*dbp;
{

	dbp->d_current++;
	return( dbp->d_current <= dbp->d_last ? dbp : NULL );
}

int	DB_getseq( dbp, locus, s_sbuf, sbuf )
DBASE_T	*dbp;
char	locus[];
int	s_sbuf;
char	sbuf[];
{
	int	rval, iref;
	int	slen;
	char	*sp, *ep;

	*locus = '\0';
	*sbuf = '\0';
	if( dbp->d_current > dbp->d_last )
		return( 0 );

	if( dbp->d_type == DB_STD )
		iref = dbp->d_current;
	else
		iref = dbp->d_iref[ dbp->d_current ];
	rval = GB_get_entry( iref, n_fmap, fmap,
		ebuf, &gentry, gbrefs, gbftab, gbqtab );
	if( rval )
		return( 0 );

	sscanf( &ebuf[ gentry.g_locus ], "%s", locus );

	for( slen = 0, sp = sbuf, ep = &ebuf[ gentry.g_sequence ]; *ep; ep++ ){
		if( isalpha( *ep ) ){
			if( slen < s_sbuf - 1 )
				*sp++ = *ep;
			slen++;
		}
	}
	*sp = '\0';
	if( slen > s_sbuf - 1 ){
		fprintf( stderr,
			"DB_getseq: sequence '%s' too long, truncated.\n",
			locus );
	}
	return( sp - sbuf );
}
