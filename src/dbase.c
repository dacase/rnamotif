#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "rnamot.h"

#include "genbank.h"

#define	FMAP_SIZE	100
static	GB_FMAP_T	fmap[ FMAP_SIZE ];
static	int		n_fmap;

static	char	s_dbname[ 256 ];
static	char	*s_dbnp;

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
	int	first, last;
	DBASE_T	*dbp;

	if( n_fmap == 0 ){
		n_fmap = GB_read_fmap( NULL, FMAP_SIZE, fmap );
		if( n_fmap == 0 ){
			fprintf( stderr, "DB_open: can't read file map.\n" );
			return( NULL );
		}
	}

fprintf( stderr, "DB_open: n_fmap = %d\n", n_fmap );

	first = last = UNDEF;
	for( fmp = fmap, i = 0; i < n_fmap; i++, fmp++ ){
		if( !strcmp( fmp->f_dbname, dbname ) ){
			if( first == UNDEF )
				first = fmp->f_dbfirst;
			last = fmp->f_dblast;
		}
	}

fprintf( stderr, "DB_open: dbname = '%s', first = %d, last = %d\n",
	dbname, first, last );

	if( first == UNDEF ){
		fprintf( stderr, "DB_open: can't read database '%s'.\n",
			dbname );
		return( NULL );
	}

	dbp = ( DBASE_T * )malloc( sizeof( DBASE_T ) );
	if( dbp == NULL ){
		fprintf( stderr, "DB_open: can't allocate dbp.\n" );
		return( NULL );
	}
	dbp->d_first = first;
	dbp->d_last = last;
	dbp->d_current = first;

	return( dbp );
}

DBASE_T	*DB_next()
{

	return( NULL );
}

int	DB_getseq( dbp, s_sbuf, sbuf )
DBASE_T	*dbp;
int	s_sbuf;
char	sbuf[];
{
	int	rval;
	int	slen;
	char	*sp, *ep;

	*sbuf = 0;
	if( dbp->d_current > dbp->d_last )
		return( 0 );

	rval = GB_get_entry( dbp->d_current, n_fmap, fmap,
		ebuf, &gentry, gbrefs, gbftab, gbqtab );
	dbp->d_current++;
	if( rval )
		return( 0 );

	for( slen = 0, sp = sbuf, ep = &ebuf[ gentry.g_sequence ]; *ep; ep++ ){
		if( isalpha( *ep ) ){
			if( slen < s_sbuf - 1 )
				*sp++ = *ep;
			slen++;
		}
	}
	*sp = '\0';
	return( sp - sbuf );
}
