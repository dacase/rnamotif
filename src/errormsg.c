#include <stdio.h>

extern	char	rmfname[];
extern	int	rmlineno;

void	errormsg( fatal, msg )
int	fatal;
char	msg[];
{

	fprintf( stderr, "%s:%d %s\n", rmfname, rmlineno, msg );
	if( fatal )
		exit( 1 );
}
