#include <stdio.h>
#include <ctype.h>
#include <string.h>

static	int	skipbl2nl();

int	FN_fgetseq( fp, sid, s_sdef, sdef, s_sbuf, sbuf )
FILE	*fp;
char	sid[];
int	s_sdef;
char	sdef[];
int	s_sbuf;
char	sbuf[];
{
	char	*dp, *sp;
	int	c, cnt;

	*sid = '\0';
	*sdef = '\0';
	*sbuf = '\0';
	
	if( ( c = getc( fp ) ) == EOF )
		return( 0 );
	else if( c != '>' ){
		fprintf( stderr,
			"FN_fgetseq: fastn file does not begin with '>'.\n" );
		return( 0 );
	}

	if( ( c = skipbl2nl( fp ) ) == EOF || c == '\n' ){
		fprintf( stderr,
			"FN_fgetseq: fastn file has unnamed entry.\n" );
		return( 0 );
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
			"FN_fgetseq: last entry: '%s': has no sequence.\n",
			sid );
		return( 0 );
	}else if( c != '\n' )
		c = skipbl2nl( fp );

	if( c != '\n' ){
		dp = sdef;
		*dp++ = c;
		for( cnt = 1; c = getc( fp ); ){
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
	if( c == EOF ){
		fprintf( stderr,
		"FN_fgetseq: last entry of fastn file has no sequence.\n" );
		return( 0 );
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
	if( cnt >= s_sbuf ){
		fprintf( stderr,
		"FN_fgetseq: entry: '%s': seq len: %d, truncated to %d.\n",
			sid, cnt, s_sbuf - 1 );
	}

	return( sp - sbuf );
}

static	int	skipbl2nl( fp )
FILE	*fp;
{
	int	c;

	while( isspace( c = getc( fp ) ) )
		if( c == '\n' )
			break;
	return( c );
}
