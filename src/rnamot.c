#include <stdio.h>

#include "rnamot.h"

main()
{

	while( yyparse() ){
		fprintf( stderr, "syntax error.\n" );
	}

	SE_dump( stderr, 1, 1, 1, 1 );
}
