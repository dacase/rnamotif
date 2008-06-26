#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#define	UNDEF	(-1)

#include "fmap.h"

static	FM_ENTRY_T	*readfme( FILE *, int );
static	int	setrange( char *, int *, int *, int );

FMAP_T	*FMread_fmap( char *fmname )
{
	FILE	*fp = NULL;
	int	n_fme;
	char	line[ 256 ], name[ 256 ], value[ 256 ];
	char	*lp, *np, *vp;
	int	g_count = 0;
	int	count = UNDEF;
	FMAP_T	*fmap = NULL;
	int	err = 0;

	if( fmname == NULL ){
		fprintf( stderr, "FMread_fmap: NULL map-file\n" );
		err = 1;
		goto CLEAN_UP;
	}

	if( ( fp = fopen( fmname, "r" ) ) == NULL ){
		fprintf( stderr,
			"FMread_fmap: can't read file-map %s\n", fmname );
		err = 1;
		goto CLEAN_UP;
	}

	fmap = ( FMAP_T * )malloc( sizeof( FMAP_T ) );
	if( fmap == NULL ){
		fprintf( stderr, "FMread_fmap: can't allocate fmap\n" );
		err = 1;
		goto CLEAN_UP;
	}
	fmap->f_root = NULL;
	fmap->f_format = NULL;
	fmap->f_nentries = UNDEF;
	fmap->f_entries = NULL;

	for( n_fme = 0; fgets( line, sizeof( line ), fp ); ){
		if( *line == '#' )
			continue;
		for( np = name, lp = line; isspace( *lp ); )
			;
		if( *lp == '\0' )
			continue;
		if( !isalpha( *lp ) ){
			fprintf( stderr,
			"FMread_fmap: SYNTAX: name must begin with letter\n" );
			err = 1;
			goto CLEAN_UP;
		}
		for( *np++ = *lp++; isalnum( *lp ); lp++ )
			*np++ = *lp;
		*np = '\0';

		for( vp = value; isspace( *lp ); lp++ )
			;
		if( *lp != '=' ){
			fprintf( stderr,
			"FMread_fmap: SYNTAX: = expected\n" );
			err = 1;
			goto CLEAN_UP;
		}else
			lp++;
		for( ; isspace( *lp ); lp++ )
			;
		if( *lp == '\0' ){
			fprintf( stderr,
			"FMread_fmap: SYNTAX: name = value\n" );
			err = 1;
			goto CLEAN_UP;
		}
		for( ; *lp != '\n' && *lp; lp++ )
			*vp++ = *lp;
		*vp = '\0';

		if( !strcmp( name, "root" ) ){
			if( fmap->f_root != NULL ){
				fprintf( stderr,
			"FMread_fmap: SYNTAX: only one root = stmt allowed\n" );
				err = 1;
				goto CLEAN_UP;
			}
			fmap->f_root = strdup( value );
			if( fmap->f_root == NULL ){
				fprintf( stderr,
					"FMread_fmap: can't strdup root\n" );
				err = 1;
				goto CLEAN_UP;
			}
		}else if( !strcmp( name, "format" ) ){
			if( fmap->f_format != NULL ){
				fprintf( stderr,
		"FMread_fmap: SYNTAX: only one format = stmt allowed\n" );
				err = 1;
				goto CLEAN_UP;
			}
			fmap->f_format = strdup( value );
			if( fmap->f_format == NULL ){
				fprintf( stderr,
					"FMread_fmap: can't strdup format\n" );
				err = 1;
				goto CLEAN_UP;
			}
		}else if( !strcmp( name, "count" ) ){
			if( g_count ){
				fprintf( stderr,
		"FMread_fmap: SYNTAX: only one count = stmt allowed\n" );
				err = 1;
				goto CLEAN_UP;
			}
			g_count = 1;
			count = atoi( value );
			if( count < 1 ){
				fprintf( stderr,
					"FMread_fmap: count must be > 0\n" );
				err = 1;
				goto CLEAN_UP;
			}
			fmap->f_nentries = count;
		}else if( !strcmp( name, "files" ) ){
			if( fmap->f_entries != NULL ){
				fprintf( stderr,
		"FMread_fmap: SYNTAX: only one files = stmt allowed\n" );
				err = 1;
				goto CLEAN_UP;
			}
			if( count == UNDEF ){
				fprintf( stderr,
		"FMread_fmap: count = stmt must precede files = stmt\n" );
				err = 1;
				goto CLEAN_UP;
			}
			fmap->f_entries = readfme( fp, count );
			if( fmap->f_entries == NULL ){
				err = 1;
				goto CLEAN_UP;
			}
		}else{
			fprintf( stderr,
				"FMread_fmap: unknown keyword '%s'\n", name );
			err = 1;
			goto CLEAN_UP;
		}
	}
	fclose( fp );
	fp = NULL;

CLEAN_UP : ;

	if( fp != NULL ){
		fclose( fp );
		fp = NULL;
	}

	if( err && fmap != NULL )
		fmap = FMfree_fmap( fmap );

	return( fmap );
}

FM_ENTRY_T	*FMget_fmentry( FMAP_T *fmap, char *dname, int part )
{
	int	i, j, k, cv;
	FM_ENTRY_T	*fme;

	for( i = 0, j = fmap->f_nentries - 1; i <= j; ){
		k = ( i + j ) / 2;
		fme = &fmap->f_entries[ k ];
		if( ( cv = strcmp( fme->f_dname, dname ) ) == 0 ){
			if( part != UNDEF ){
				cv = fme->f_part - part;
				return( &fmap->f_entries[ k - cv ] );
			}else{
				for( ; k > 0; k-- ){
					fme = &fmap->f_entries[ k - 1 ];
					if( strcmp( fme->f_dname, dname ) )
						return( &fmap->f_entries[ k ] );
				}
				return( &fmap->f_entries[ 0 ] );
			}
		}else if( cv < 0 )
			i = k + 1;
		else
			j = k - 1;
	}
	return( NULL );
}

int	FMmark_active( FMAP_T *fmap, char *dbname, int active[] )
{
	char	*dp, *pp, *pc;
	char	dname[ 32 ];
	char	range[ 32 ];
	int	f, pcnt, p, p1;
	int	pl, ph;
	FM_ENTRY_T	*fme;

	dp = dbname;
	if( pp = strchr( dp, ':' ) ){
		strncpy( dname, dp, pp - dp );
		dname[ pp - dp ] = '\0';
	}else
		strcpy( dname, dp );

	if( !strcmp( dname, "all" ) ){
		for( f = 0; f < fmap->f_nentries; f++ )
			active[ f ] = 1;
		return( 0 );
	}

	if( ( fme = FMget_fmentry( fmap, dname, UNDEF ) ) == NULL ){
		fprintf( stderr, "FMmark_active: unknown db '%s'", dp );
		return( 1 );
	}

	for( pcnt = 0, p1 = p = fme - fmap->f_entries; ; ){
		pcnt++;
		if( p == fmap->f_nentries - 1 )
			break;
		p++;
		fme++;
		if( strcmp( fme->f_dname, dname ) )
			break;
	}

	if( pp == NULL || pp[1] == '\0' ){
		for( p = 0; p < pcnt; p++ )
			active[ p1 + p ] = 1;
		return( 0 );
	}

	for( ++pp; pp && *pp; ){
		if( pc = strchr( pp, ',' ) ){
			strncpy( range, pp, pc - pp );
			range[ pc - pp ] = '\0';
			pp = pc + 1;
		}else{
			strcpy( range, pp );
			pp = pc;
		}
		if( setrange( range, &pl, &ph, pcnt ) )
			return( 1 );
		for( p = pl - 1; p < ph; p++ )
			active[ p1 + p ] = 1;
	}

	return( 0 );
}

static	int	setrange( char *range, int *pl, int *ph, int pcnt )
{
	char	*rp;
	int	rng;

	rng = 0;
	rp = range;
	if( !isdigit( *rp ) && *rp != '-' ){
		fprintf( stderr, "setrange: SYNTAX: N, N-M, N-, -M, -" );
		return( 1 );
	}
	for( *pl = 0; isdigit( *rp ); rp++ )
		*pl = 10 * *pl + *rp - '0';
	if( *pl == 0 )
		*pl = 1;
	if( *rp == '-' ){
		rp++;
		rng = 1;
	}
	for( *ph = 0; isdigit( *rp ); rp++ )
		*ph = 10 * *ph + *rp - '0';
	if( *rp != '\0' ){
		fprintf( stderr, "setrange: SYNTAX: extra chars" );
		return( 1 );
	}
	if( *ph == 0 )
		*ph = rng ? pcnt : *pl;
	if( *ph > pcnt )
		*ph = pcnt;
	if( *ph < *pl ){
		fprintf( stderr, "setrange: low (%d) > hi (%d)", *pl, *ph );
		return( 1 );
	}

	return( 0 );
}

void	FMwrite_fmap( FILE *fp, FMAP_T *fmap )
{
	int	f;
	FM_ENTRY_T	*fme;

	fprintf( fp, "root = %s\n", fmap->f_root );
	fprintf( fp, "format = %s\n", fmap->f_format );
	fprintf( fp, "count = %d\n", fmap->f_nentries );
	fprintf( fp, "files = {\n" );
	for( fme = fmap->f_entries, f = 0; f < fmap->f_nentries; f++, fme++ ){
		fprintf( fp, "\t%-4s %3d %s",
			fme->f_dname, fme->f_part, fme->f_fname );
		if( fme->f_hosts )
			fprintf( fp, " %s", fme->f_hosts );
		fprintf( fp, "\n" );
	}
	fprintf( fp, "}\n" );
}

void	*FMfree_fmap( FMAP_T *fmap )	
{
	FM_ENTRY_T	*fme;
	int	f;

	if( fmap != NULL ){
		if( fmap->f_root != NULL )
			free( fmap->f_root );
		if( fmap->f_format != NULL )
			free( fmap->f_format );
		if( ( fme = fmap->f_entries ) != NULL ){
			for( f = 0; f < fmap->f_nentries; f++, fme++ ){
				if( fme->f_dname != NULL )
					free( fme->f_dname );
				if( fme->f_fname != NULL )
					free( fme->f_fname );
				if( fme->f_hosts != NULL )
					free( fme->f_hosts );
			}
			free( fmap->f_entries );
		}
		free( fmap );
		fmap = NULL;
	}
	return( NULL );
}

static	FM_ENTRY_T	*readfme( FILE *fp, int count )
{
	FM_ENTRY_T	*fme = NULL, *fme1;
	char	line[ 256 ];
	int	c, nf, f;
	char	*fields[ 20 ];
	int	done, err = 0;

	fme = ( FM_ENTRY_T * )calloc( ( long )count, sizeof( FM_ENTRY_T ) );
	if( fme == NULL ){
		fprintf( stderr, "readfme: can't allocate fme\n" );
		return( NULL );
	}

	for( done = 0, fme1 = fme, c = 0; fgets( line, sizeof( line ), fp ); ){
		if( *line == '#' )
			continue;
		nf = split( line, fields, " \t\n" );
		if( nf == 0 )
			continue;
		else if( nf == 1 ){
			if( !strcmp( fields[ 0 ], "}" ) )
				done = 1;
			else{
				fprintf( stderr,
				"readfme: SYNTAX: line beginning '%s' ...\n",
					fields[ 0 ] );
				err = 1;
				done = 1;
			}
		}else if ( nf == 3 || nf == 4 ){
			if( c < count ){
				fme1->f_dname = fields[ 0 ];
				fields[ 0 ] = NULL;
				fme1->f_part = atoi( fields[ 1 ] );
				fme1->f_fname = fields[ 2 ];
				fields[ 2 ] = NULL;
				if( nf == 4 ){
					fme1->f_hosts = fields[ 3 ];
					fields[ 3 ] = NULL;
				}
				fme1++;
			}
			c++;
		}else{
			fprintf( stderr,
				"readfme: SYNTAX: line beginning '%s' ...\n",
				fields[ 0 ] );
			err = 1;
			done = 1;
		}
		for( f = 0; f < nf; f++ ){
			if( fields[ f ] != NULL )
				free( fields[ f ] );
		}
		if( done )
			break;
	}

	if( c != count ){
		fprintf( stderr,
			"readfme: fmap has %d entries, expecting %d\n",
			c, count );
		err = 1;
	}

	if( err ){
		for( fme1 = fme, f = 0; f < count; f++, fme1++ ){
			if( fme->f_dname != NULL )
				free( fme->f_dname );
			if( fme->f_fname != NULL )
				free( fme->f_fname );
			if( fme->f_hosts != NULL )
				free( fme->f_hosts );
		}
		free( fme );
		fme = NULL;
	}

	return( fme );
}
