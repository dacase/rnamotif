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

	/* Ctl-A separates entries on the ID line of a fastn file	*/
	/* So this may be wrong in that it returns only the first	*/
	/* such def if there are more than 1				*/

	elp = strpbrk( line, "\001\r\n" );
	if( elp == NULL ){
		fprintf( stderr, "FN_fgetseq: bad fastn entry: '%s'.\n", line );
		skip( fp );
		return( 0 );
	}

	sp = strchr( line, ' ' );
	if( sp == NULL ){
		strncpy( sid, &line[ 1 ], elp - line - 1 );
		sid[ elp - line - 1 ] = '\0';
	}else{
		strncpy( sid, &line[ 1 ], sp - line - 1 );
		sid[ sp - line - 1 ] = '\0';
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
