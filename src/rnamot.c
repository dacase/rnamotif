#include <stdio.h>

#include "rnamot.h"

extern	int	yydebug;

main( argc, argv )
int	argc;
char	*argv[];
{

	RM_init();

	if( yyparse() ){
		fprintf( stderr, "syntax error.\n" );
	}

	RM_dump( stderr, 1, 1, 1 );
}
