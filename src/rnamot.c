#include <stdio.h>

#include "rnamot.h"

int	rm_error;
VALUE_T	rm_tokval;
int	rm_lineno;
int	rm_emsg_lineno;
char	rm_fname[ 256 ] = "--stdin--";

#define	RM_GLOBAL_IDS_SIZE	50
IDENT_T	rm_global_ids[ RM_GLOBAL_IDS_SIZE ] = {
	{ "wc", T_PAIR, C_VAR, S_GLOBAL, { T_PAIR, NULL } },
	{ "gu", T_PAIR, C_VAR, S_GLOBAL, { T_PAIR, NULL } },
	{ "tr", T_PAIR, C_VAR, S_GLOBAL, { T_PAIR, NULL } },
	{ "qu", T_PAIR, C_VAR, S_GLOBAL, { T_PAIR, NULL } },
	{ "overlap", T_INT, C_VAR, S_GLOBAL, { T_INT, 0 } },
	{ "database", T_STRING, C_VAR, S_GLOBAL, { T_STRING, NULL } }
};
int	rm_s_global_ids = RM_GLOBAL_IDS_SIZE;;
int	rm_n_global_ids = 6;

#define	RM_DESCR_SIZE 100
STREL_T	rm_descr[ RM_DESCR_SIZE ];
int	rm_s_descr = RM_DESCR_SIZE;
int	rm_n_descr;

extern	int	yydebug;

main( argc, argv )
int	argc;
char	*argv[];
{

	RM_init();

	if( yyparse() ){
		fprintf( stderr, "syntax error.\n" );
	}
	if( !rm_error ){
		if( SE_link( rm_n_descr, rm_descr ) )
			exit( 1 );
	}

	RM_dump( stderr, 1, 1, 1 );
}
