#include <stdio.h>

#include "rnamot.h"

int	rm_error;
VALUE_T	rm_tokval;
int	rm_lineno;
int	rm_emsg_lineno;
char	rm_fname[ 256 ] = "--stdin--";
int	rm_copt = 0;
int	rm_dopt = 0;

#define	RM_GLOBAL_IDS_SIZE	50
IDENT_T	rm_global_ids[ RM_GLOBAL_IDS_SIZE ] = {
	{ "wc", T_PAIR, C_VAR, S_GLOBAL, { T_PAIR, NULL } },
	{ "gu", T_PAIR, C_VAR, S_GLOBAL, { T_PAIR, NULL } },
	{ "tr", T_PAIR, C_VAR, S_GLOBAL, { T_PAIR, NULL } },
	{ "qu", T_PAIR, C_VAR, S_GLOBAL, { T_PAIR, NULL } },
	{ "overlap", T_INT, C_VAR, S_GLOBAL, { T_INT, 0 } },
	{ "database", T_STRING, C_VAR, S_GLOBAL, { T_STRING, "VRT" } }
};
int	rm_s_global_ids = RM_GLOBAL_IDS_SIZE;
int	rm_n_global_ids = 6;

#define	RM_DESCR_SIZE 100
STREL_T	rm_descr[ RM_DESCR_SIZE ];
int	rm_s_descr = RM_DESCR_SIZE;
int	rm_n_descr;

#define	RM_POS_SIZE	10
POS_T	rm_pos[ RM_POS_SIZE ];
int	rm_s_pos = RM_POS_SIZE;
int	rm_n_pos;

SITE_T	*rm_sites = NULL;

extern	int	yydebug;

char	*getenv();

#define	SBUFSIZE	2000000
static	char	sbuf[ SBUFSIZE ];
static	int	slen;

IDENT_T	*find_id();

DBASE_T	*DB_open();
DBASE_T *DB_next();

main( argc, argv )
int	argc;
char	*argv[];
{
	DBASE_T	*dbp;
	IDENT_T	*ip;
	char	*dbnp;
	char	locus[ 20 ];

	RM_init( argc, argv );

	if( yyparse() ){
		errormsg( 1, "syntax error." );
	}
	if( !rm_error ){
		if( SE_link( rm_n_descr, rm_descr ) )
			exit( 1 );
	}

	if( rm_dopt )
		RM_dump( stderr, 1, 1, 1 );

	if( rm_error )
		exit( 1 );

	if( rm_copt )
		exit( 0 );

	ip = find_id( "database" );
	if( ip == NULL ){
		fprintf( stderr, "rnamot: 'database' not defined.\n" );
		exit( 1 );
	}else
		dbnp = ip->i_val.v_value.v_pval;

	for( dbp = DB_open( dbnp ); dbp; ){
		if( slen = DB_getseq( dbp, locus, SBUFSIZE, sbuf ) ){
			find_motif( rm_n_descr, rm_descr, rm_sites,
				locus, slen, sbuf );
		}
		dbp = DB_next( dbp );
	}

	exit( 0 );
}
