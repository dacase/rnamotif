#include <stdio.h>

#include "rnamot.h"

#include "dbase.h"

extern	int	rm_error;
extern	char	rm_dfname[];
extern	int	rm_copt;
extern	int	rm_dopt;
extern	int	rm_hopt;
extern	FILE	*rm_dbfp;
extern	int	rm_dtype;

extern	STREL_T	rm_descr[];
extern	int	rm_n_descr;
extern	int	rm_dminlen;	/* min. len. of entire motif	*/
extern	int	rm_dmaxlen;	/* max. len. of entire motif	*/

extern	SITE_T	*rm_sites;

extern	SEARCH_T	**rm_searches;
extern	int		rm_n_searches;

#define	SBUF_SIZE	2000000
static	char	sbuf[ SBUF_SIZE ];
static	int	slen;
static	char	csbuf[ SBUF_SIZE ];

IDENT_T	*find_id();

static	void	mk_rcmp();

main( argc, argv )
int	argc;
char	*argv[];
{
	DBASE_T	*dbp;
	IDENT_T	*ip;
	char	*dbnp;
	char	locus[ 20 ];

	if( RM_init( argc, argv ) )
		exit( 1 );

	if( yyparse() ){
		errormsg( 1, "syntax error." );
	}
	if( !rm_error ){
		if( SE_link( rm_n_descr, rm_descr ) )
			exit( 1 );
		fprintf( stderr, "%s: complete descr length: min/max = %d/",
			rm_dfname, rm_dminlen );
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
		if( slen = DB_getseq( dbp, locus, SBUF_SIZE, sbuf ) ){
			find_motif_driver( rm_n_searches, rm_searches, rm_sites,
				locus, 0, slen, sbuf );
			mk_rcmp( slen, sbuf, csbuf );
			find_motif_driver( rm_n_searches, rm_searches, rm_sites,
				locus, 1, slen, csbuf );
		}
		dbp = DB_next( dbp );
	}
/*
	while( slen = DB_getseq( stdin, locus, SBUF_SIZE, sbuf ) ){
		find_motif_driver( rm_n_searches, rm_searches, rm_sites,
			locus, 0, slen, sbuf );
		mk_rcmp( slen, sbuf, csbuf );
		find_motif_driver( rm_n_searches, rm_searches, rm_sites,
			locus, 1, slen, csbuf );
	}
*/

	exit( 0 );
}

static	void	init_rcmp()
{
	int	i;

}

static	void	mk_rcmp( slen, sbuf, csbuf )
int	slen;
char	sbuf[];
char	csbuf[];
{
	static	int	init = 0;
	static	char	wc_cmp[ 128 ];
	char	*sp, *cp;
	int	i;

	if( !init ){
		init = 1;
		for( i = 0; i < 128; i++ )
			wc_cmp[ i ] = 'n';
		wc_cmp[ 'a' ] = 't'; wc_cmp[ 'A' ] = 't';
		wc_cmp[ 'c' ] = 'g'; wc_cmp[ 'C' ] = 'g';
		wc_cmp[ 'g' ] = 'c'; wc_cmp[ 'G' ] = 'c';
		wc_cmp[ 't' ] = 'a'; wc_cmp[ 'T' ] = 'a';
		wc_cmp[ 'u' ] = 'a'; wc_cmp[ 'U' ] = 'a';
	}

	csbuf[ slen ] = '\0';
	for( sp = sbuf, cp = &csbuf[ slen - 1 ], i = 0; i < slen; i++ )
		*cp-- = wc_cmp[ *sp++ ];
}
