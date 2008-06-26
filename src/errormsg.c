#include <stdio.h>
#include <stdlib.h>

#include "rmdefs.h"

extern	int	rm_error;
extern	char	*rm_wdfname;
extern	int	rm_emsg_lineno;

void	RM_errormsg( int fatal, char msg[] )
{

	rm_error = TRUE;
	fprintf( stderr, "%s:%d %s\n", rm_wdfname, rm_emsg_lineno, msg );
	if( fatal )
		exit( 1 );
}
