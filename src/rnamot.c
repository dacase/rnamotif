#include <stdio.h>

#include "rnamot.h"

#include "dbase.h"

extern	int	rm_error;
extern	char	rm_fname[];
extern	int	rm_copt;
extern	int	rm_dopt;
extern	int	rm_hopt;

extern	STREL_T	rm_descr[];
extern	int	rm_n_descr;
extern	int	rm_dminlen;	/* min. len. of entire motif	*/
extern	int	rm_dmaxlen;	/* max. len. of entire motif	*/

extern	SITE_T	*rm_sites;

extern	SEARCH_T	**rm_searches;
extern	int		rm_n_searches;

#define	SBUFSIZE	2000000
static	char	sbuf[ SBUFSIZE ];
static	int	slen;

IDENT_T	*find_id();

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
		fprintf( stderr, "%s: complete descr length: min/max = %d/",
			rm_fname, rm_dminlen );
		if( rm_dmaxlen == UNBOUNDED )
			fprintf( stderr, "UNBND\n" );
		else
			fprintf( stderr, "%d\n", rm_dmaxlen );
	}

	if( rm_dopt || rm_hopt )
		RM_dump( stderr, rm_dopt, rm_dopt, rm_dopt, rm_hopt );

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
			find_motif_driver( rm_n_searches, rm_searches, rm_sites,
				locus, slen, sbuf );
		}
		dbp = DB_next( dbp );
	}

	exit( 0 );
}
