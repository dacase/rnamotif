#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int	split( char str[], char *fields[], char *fsep )
{
	int	nf, flen, white;
	char	*sp, *fp, *efp, *nfp;

	if( !str )
		return( 0 );

	/* Use fsep of white space is special				*/ 
	/* although a \n is a white space character, a fsep of a	*/
	/* single \n is not considered white space, allowing multi line	*/
	/* strings to be broken into lines				*/

	if( !strcmp( fsep, "\n" ) )
		white = 0;
	else if( strspn( fsep, " \t\n" ) == strlen( fsep ) ){
		white = 1;
	}else
		white = 0;

	if( white ){
		for( nf = 0, sp = str; ; ){
			fp = sp + strspn( sp, fsep );
			if( !*fp )
				return( nf );
			if( efp = strpbrk( fp, fsep ) ){
				if( !( flen = efp - fp ) )
					return( nf );
				nfp = (char *)malloc((flen + 1) * sizeof(char));
				strncpy( nfp, fp, flen );
				nfp[ flen ] = '\0';
				fields[ nf ] = nfp;
				sp = efp;
				nf++;
			}else{
				flen = strlen( fp );
				nfp = (char *)malloc((flen + 1) * sizeof(char));
				strcpy( nfp, fp );
				fields[ nf ] = nfp;
				nf++;
				return( nf );
			}
		}
	}else{
		for( nf = 0, sp = str; ; ){
			if( fp = strchr( sp, *fsep ) ){
				flen = fp - sp;
				nfp = (char *)malloc((flen + 1) * sizeof(char));
				strncpy( nfp, sp, flen );
				nfp[ flen ] = '\0';
				fields[ nf ] = nfp;
				nf++;
				sp = fp + 1;
			}else{
				flen = strlen( sp );
				nfp = (char *)malloc((flen + 1) * sizeof(char));
				strcpy( nfp, sp );
				fields[ nf ] = nfp;
				nf++;
				return( nf );
			}
		}
	}
}
