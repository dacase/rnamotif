#include <stdio.h>
#include <ctype.h>
#include <string.h>

int	FN_fgetseq( fp, locus, s_sbuf, sbuf )
FILE	*fp;
char	locus[];
int	s_sbuf;
char	sbuf[];
{
	char	line[ 1024 ];
	char	*elp, *lp, *sp;
	int	c;

	*locus = '\0';
	*sbuf = '\0';
	if( fgets( line, sizeof( line ), fp ) == NULL )
		return( 0 ); 

	elp = strpbrk( line, " \r\n" );
	if( elp == NULL ){
		fprintf( stderr, "bad fastn entry: %s", line );
		skip();
		return( 0 );
	}

	for( lp = elp - 1; lp >= line && *lp != '|'; lp-- )
		;
	if( *lp == '|' )
		lp++;
	if( elp - lp == 0 ){
		elp = lp - 1;
		for( lp -= 2; lp >= line && *lp != '|'; lp-- ) 
			;
		if( *lp == '|' )
			lp++;
	}
	strncpy( locus, lp, elp - lp );
	locus[ elp - lp ] = '\0';

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

int	skip()
{

}
