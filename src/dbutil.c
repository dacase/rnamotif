#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "rnamot.h"

static	int	skipbl2nl( FILE * );

FILE	*DB_fnext( FILE *fp, int *c_fname, int n_fname, char *fname[] )
{

/*
	if( fp != NULL && fp != stdin )
		fclose( fp );
*/
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
	int	c, cnt;
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

	for( cnt = 0, sp = sbuf; ( c = getc( fp ) ) != EOF; ){
		if( c == '>' ){
			ungetc( c, fp );
			break;
		}else if( isalpha( c ) ){
			cnt++;
			if( cnt < s_sbuf ){
				c = tolower( c );
				*sp++ = c == 'u' ? 't' : c;
			}
		}
	}
	*sp = '\0';
	if( cnt > s_sbuf ){
		fprintf( stderr,
		"FN_fgetseq: entry: '%s': seq len: %d, truncated to %d.\n",
			sid, cnt, s_sbuf - 1 );
	}

	return( sp - sbuf );
}

int	PIR_fgetseq( FILE *fp, char sid[], int s_sdef, char sdef[],
	int	s_sbuf, char sbuf[] )
{
	int	c, cnt;
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

	for( cnt = 0, sp = sbuf; ( c = getc( fp ) ) != EOF; ){
		if( c == '>' ){
			ungetc( c, fp );
			break;
		}else if( isalpha( c ) ){
			cnt++;
			if( cnt < s_sbuf ){
				c = tolower( c );
				*sp++ = c == 'u' ? 't' : c;
			}
		}
	}
	*sp = '\0';
	if( cnt > s_sbuf ){
		fprintf( stderr,
		"PIR_fgetseq: entry: '%s': seq len: %d, truncated to %d.\n",
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