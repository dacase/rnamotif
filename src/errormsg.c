#include <stdio.h>

extern	int	rm_error;
extern	char	rm_dfname[];
extern	int	rm_emsg_lineno;

void	RM_errormsg( fatal, msg )
int	fatal;
char	msg[];
{

	rm_error = 1;
	fprintf( stderr, "%s:%d %s\n", rm_dfname, rm_emsg_lineno, msg );
	if( fatal )
		exit( 1 );
}
