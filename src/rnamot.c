#include <stdio.h>

#include "rnamot.h"

main()
{

	while( yyparse() ){
		fprintf( stderr, "syntax error.\n" );
	}
}
