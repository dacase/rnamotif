#include <stdio.h>
#include <ctype.h>
#include <string.h>

static	char	line[ 10240 ];

static	void	skip();

int	FN_fgetseq( fp, sid, sdef, s_sbuf, sbuf )
FILE	*fp;
char	sid[];
char	sdef[];
int	s_sbuf;
char	sbuf[];
{
	char	*elp, *lp, *sp;
	int	c;

	*sid = '\0';
	*sdef = '\0';
	*sbuf = '\0';
	if( fgets( line, sizeof( line ), fp ) == NULL )
		return( 0 ); 

	elp = strpbrk( line, "\r\n" );
	if( elp == NULL ){
		fprintf( stderr, "FN_fgetseq: bad fastn entry: '%s'.\n", line );
		skip( fp );
		return( 0 );
	}

	for( lp = &line[ 1 ]; isspace( *lp ); lp++ )
		;
	sp = strchr( lp, ' ' );
	if( sp == NULL ){
		strncpy( sid, lp, elp - line );
		sid[ elp - line ] = '\0';
	}else{
		strncpy( sid, lp, sp - line );
		sid[ sp - lp ] = '\0';
		while( *sp == ' ' )
			sp++;
		strncpy( sdef, sp, elp - sp );
		sdef[ elp - sp ] = '\0';
	}

	for( sp = sbuf; ( c = getc( fp ) ) != EOF; ){
		if( c == '>' ){
			ungetc( c, fp );
			break;
		}else if( isalpha( c ) )
			*sp++ = tolower( c );
	}
	*sp = '\0';

	return( sp - sbuf );
}

static	void	skip( fp )
FILE	*fp;
{
	int	c;

	while( ( c = getc( fp ) ) != '>' && c != EOF )
		;
	if( c == '>' )
		ungetc( c, fp );
}
