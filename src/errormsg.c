#include <stdio.h>

extern	char	rmfname[];
extern	int	rmemsglineno;

void	errormsg( fatal, msg )
int	fatal;
char	msg[];
{

	fprintf( stderr, "%s:%d %s\n", rmfname, rmemsglineno, msg );
	if( fatal )
		exit( 1 );
}
