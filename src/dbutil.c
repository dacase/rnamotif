#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "rmdefs.h"
#include "dbutil.h"

#define	MAXCNT	0x7fffffff

static	int	skipbl2nl( FILE * );

FILE	*DB_fnext( FILE *fp, int *c_fname, int n_fname, char *fname[] )
{

	/* Initial state is NULL; called w/stdin means we're done.	*/
	if( fp == stdin )
		return( NULL );
	else if( fp != NULL )
		fclose( fp );

	if( n_fname == 0 )
		return( stdin );
	else if( *c_fname == UNDEF ){
		*c_fname = 0;
		if( ( fp = fopen( fname[ *c_fname ], "r" ) ) == NULL ){
			fprintf( stderr,
				"DB_fnext: can't read seq file '%s'.\n",
				fname[ *c_fname ] );
		}
	}else if( *c_fname < n_fname - 1 ){
		( *c_fname )++;
		if( ( fp = fopen( fname[ *c_fname ], "r" ) ) == NULL ){
			fprintf( stderr,
				"DB_fnext: can't read seq file '%s'.\n",
				fname[ *c_fname ] );
		}
	}else
		fp = NULL;
	return( fp );
}

int	FN_fgetseq( FILE *fp, char sid[], int s_sdef, char sdef[],
	int	s_sbuf, char sbuf[] )
{
	int	c;
	unsigned int	cnt;
	int	ovfl;
	char	*dp, *sp;

	*sid = '\0';
	*sdef = '\0';
	*sbuf = '\0';

	if( ( c = getc( fp ) ) == EOF )
		return( EOF );
	else if( c != '>' ){
		fprintf( stderr,
			"FN_fgetseq: fastn file does not begin with '>'.\n" );
		return( EOF );
	}

	if( ( c = skipbl2nl( fp ) ) == EOF || c == '\n' ){
		fprintf( stderr,
			"FN_fgetseq: fastn file has an unnamed entry.\n" );
		return( EOF );
	}

	dp = sid;
	for( *dp++ = c; ( c = getc( fp ) ) != EOF; ){
		if( !isspace( c ) )
			*dp++ = c;
		else
			break;
	}
	*dp = '\0';
	if( c == EOF )
		return( 0 );
	if( c != '\n' ){
		if( ( c = skipbl2nl( fp ) ) == EOF )
			return( 0 );
	}
	if( c != '\n' ){
		dp = sdef;
		for( cnt = 1, *dp++ = c; c = getc( fp ); ){
			if( c == '\n' || c == EOF )
				break;
			cnt++;
			if( cnt < s_sdef )
				*dp++ = c;
		}
		*dp = '\0';
		if( cnt >= s_sdef ){
			fprintf( stderr,
		"FN_fgetseq: entry: '%s': def len: %d, truncated to %d.\n",
				sid, cnt, s_sdef - 1 );
		}
	}
	if( c == EOF )
		return( 0 );

	for( ovfl = 0, cnt = 0, sp = sbuf; ( c = getc( fp ) ) != EOF; ){
		if( c == '>' ){
			ungetc( c, fp );
			break;
		}else if( isalpha( c ) ){
			if( cnt < MAXCNT ){
				cnt++;
				if( cnt < s_sbuf ){
					c = tolower( c );
					*sp++ = c == 'u' ? 't' : c;
				}
			}else
				ovfl = 1;
		}
	}
	*sp = '\0';
	if( ovfl ){
		fprintf( stderr,
	"FN_fgetseq: entry: '%s': seq len > 2^31-1 (%d), truncated to %d.\n",
			sid, MAXCNT, s_sbuf - 1 );
	}else if( cnt > s_sbuf ){
		fprintf( stderr,
		"FN_fgetseq: entry: '%s': seq len: %d, truncated to %d.\n",
			sid, cnt, s_sbuf - 1 );
	}

	return( sp - sbuf );
}

int	PIR_fgetseq( FILE *fp, char sid[], int s_sdef, char sdef[],
	int	s_sbuf, char sbuf[] )
{
	int	c;
	unsigned int	cnt;
	int	ovfl;
	char	*dp, *sp;

	*sid = '\0';
	*sdef = '\0';
	*sbuf = '\0';

	if( ( c = getc( fp ) ) == EOF )
		return( EOF );
	else if( c != '>' ){
		fprintf( stderr,
			"PIR_fgetseq: pir file does not begin with '>'.\n" );
		return( EOF );
	}

	if( ( c = skipbl2nl( fp ) ) == EOF || c == '\n' ){
		fprintf( stderr,
			"PIR_fgetseq: pir file has an unnamed entry.\n" );
		return( EOF );
	}

	dp = sid;
	for( *dp++ = c; ( c = getc( fp ) ) != EOF; ){
		if( !isspace( c ) )
			*dp++ = c;
		else
			break;
	}
	*dp = '\0';
	if( c == EOF ){
		fprintf( stderr,
			"PIR_fgetseq: entry: '%s': no title line.\n", sid );
		return( EOF );
	}else if( c != '\n' ){
		fprintf( stderr,
		"PIR_fgetseq: entry: '%s': extra chars on ID line ignored.\n",
			sid );
		while( ( c = getc( fp ) ) != EOF ){
			if( c == '\n' )
				break;
		}
	}
	if( c != '\n' ){
		fprintf( stderr,
			"PIR_fgetseq: entry: '%s': no title line.\n", sid );
		return( EOF );
	}else
		c = getc( fp );
	dp = sdef;
	for( cnt = 1, *dp++ = c; c = getc( fp ); ){
		if( c == '\n' || c == EOF )
			break;
		cnt++;
		if( cnt < s_sdef )
			*dp++ = c;
	}
	*dp = '\0';
	if( cnt >= s_sdef ){
		fprintf( stderr,
	"PIR_fgetseq: entry: '%s': title len: %d, truncated to %d.\n",
			sid, cnt, s_sdef - 1 );
	}

	for( ovfl = 0, cnt = 0, sp = sbuf; ( c = getc( fp ) ) != EOF; ){
		if( c == '>' ){
			ungetc( c, fp );
			break;
		}else if( isalpha( c ) ){
			if( cnt < MAXCNT ){
				cnt++;
				if( cnt < s_sbuf ){
					c = tolower( c );
					*sp++ = c == 'u' ? 't' : c;
				}
			}else
				ovfl = 1;
		}
	}
	*sp = '\0';
	if( ovfl ){
		fprintf( stderr,
	"PIR_fgetseq: entry: '%s': seq len > 2^31-1 (%d), truncated to %d.\n",
			sid, MAXCNT, s_sbuf - 1 );
	}else if( cnt > s_sbuf ){
		fprintf( stderr,
		"PIR_fgetseq: entry: '%s': seq len: %d, truncated to %d.\n",
			sid, cnt, s_sbuf - 1 );
	}

	return( sp - sbuf );
}
int	GB_fgetseq( FILE *fp, char sid[], int s_sdef, char sdef[],
	int	s_sbuf, char sbuf[] )
{
	unsigned int	cnt;
	int	ovfl;
	char	line[ 256 ], locus[ 20 ], acc[ 20 ], gid[ 20 ];
	char	*lp, *dp, *sp;

	*sid = '\0';
	*sdef = '\0';
	*sbuf = '\0';

	for( *locus = '\0'; fgets( line, sizeof( line ), fp ); ){
		if( !strncmp( line, "LOCUS", 5 ) ){
			sscanf( line, "LOCUS %s", locus );
			break;
		}
	}
	if( *locus == '\0' )
		return( EOF );

	while( fgets( line, sizeof( line ), fp ) ){
		if( !strncmp( line, "DEFINITION", 10 ) ){
			for( dp = sdef, cnt = 0, lp = line; *lp; lp++ ){
				cnt++;
				if( cnt < s_sdef )
					*dp++ = *lp == '\n' ? ' ' : *lp;
			}
			*dp = '\0';
		}else if( !strncmp( line, "ACCESSION", 9 ) ){
			dp[ -1 ] = '\0';
			break;
		}else{
			for( lp = line; *lp; lp++ ){
				cnt++;
				if( cnt < s_sdef )
					*dp++ = *lp == '\n' ? ' ' : *lp;
			}
			*dp = '\0';
		}
	}
	if( *sdef == '\0' ){
		fprintf( stderr, "GB_fgetseq: missing DEFINITION line.\n" );
		return( EOF );
	}
	if( cnt >= s_sdef ){
		fprintf( stderr,
		"GB_fgetseq: entry: '%s': def len: %d, truncated to %d.\n",
			sid, cnt, s_sdef - 1 );
	}

	for( *acc = *gid = '\0'; fgets( line, sizeof( line ), fp ); ){
		if( !strncmp( line, "VERSION", 7 ) ){
			sscanf( line, "VERSION %s GI:%s", acc, gid );
			break;
		}
	}
	if( *acc == '\0' ){
		fprintf( stderr, "GB_fgetseq: missing VERSION line.\n" );
		return( EOF );
	}
	if( dp = strchr( acc, '.' ) )
		*dp = '\0';
	sprintf( sid, "gi|%s|gb|%s|%s", gid, acc, locus );

	while( fgets( line, sizeof( line ), fp ) ){
		if( !strncmp( line, "ORIGIN", 6 ) ){
			break;
		}
	}
	if( strncmp( line, "ORIGIN", 6 ) ){
		fprintf( stderr, "GB_fgetseq: missing ORIGIN line.\n" );
		return( EOF );
	}

	for( sp = sbuf, ovfl = 0, cnt = 0; fgets( line, sizeof( line ), fp ); ){
		if( !strncmp( line, "//", 2 ) )
			break;
		for( lp = line; *lp; lp++ ){
			if( isalpha( *lp ) ){
				if( cnt < MAXCNT ){
					cnt++;
					if( cnt < s_sbuf ){
						*sp++ = isupper( *lp ) ?
							tolower( *lp ) : *lp;
					}
				}else
					ovfl = 1;
			}
		}
		*sp = '\0';
	}
	if( strncmp( line, "//", 2 ) ){
		fprintf( stderr, "GB_fgetseq: missing // line.\n" );
		return( EOF );
	}
	if( ovfl ){
		fprintf( stderr,
	"GB_fgetseq: entry: '%s': seq len > 2^31-1 (%d), truncated to %d.\n",
			sid, MAXCNT, s_sbuf - 1 );
	}else if( cnt > s_sbuf ){
		fprintf( stderr,
		"GB_fgetseq: entry: '%s': seq len: %d, truncated to %d.\n",
			sid, cnt, s_sbuf - 1 );
	}

	return( sp - sbuf );
}

static	int	skipbl2nl( FILE *fp )
{
	int	c;

	while( isspace( c = getc( fp ) ) )
		if( c == '\n' )
			break;
	return( c );
}
