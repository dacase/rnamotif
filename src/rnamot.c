#include <stdio.h>

#include "rnamot.h"

extern	int	rm_error;
extern	char	rm_dfname[];
extern	int	rm_copt;
extern	int	rm_dopt;
extern	int	rm_hopt;
extern	int	rm_popt;
extern	int	rm_vopt;
extern	FILE	*rm_dbfp;
extern	int	rm_dtype;

extern	STREL_T	rm_descr[];
extern	int	rm_n_descr;
extern	int	rm_dminlen;	/* min. len. of entire motif	*/
extern	int	rm_dmaxlen;	/* max. len. of entire motif	*/

extern	SITE_T	*rm_sites;

extern	SEARCH_T	**rm_searches;
extern	int		rm_n_searches;

#define	SID_SIZE	100
static	char	sid[ SID_SIZE ];
#define	SDEF_SIZE	10000
static	char	sdef[ SDEF_SIZE ];
#define	SBUF_SIZE	5000000
static	char	sbuf[ SBUF_SIZE ];
static	int	slen;
static	char	csbuf[ SBUF_SIZE ];

IDENT_T	*RM_find_id();

#ifdef	USE_GENBANK
typedef	void	DBASE_T;
DBASE_T	*dbp, *GB_opendb();
#endif

static	void	mk_rcmp();

main( argc, argv )
int	argc;
char	*argv[];
{
	IDENT_T	*ip;
	char	*dbnp;
	int	chk_both_strs;

	if( RM_init( argc, argv ) )
		exit( 1 );

	if( rm_vopt ){
		fprintf( stderr, "%s: %s.\n", argv[ 0 ], VERSION );
		exit( 0 );
	}

	if( yyparse() ){
		errormsg( 1, "syntax error." );
	}
	if( !rm_error ){
		if( SE_link( rm_n_descr, rm_descr ) )
			exit( 1 );
		fprintf( stderr, "%s: complete descr length: min/max = %d/",
			rm_dfname, rm_dminlen );
		SC_link();
		if( rm_dmaxlen == UNBOUNDED )
			fprintf( stderr, "UNBND\n" );
		else
			fprintf( stderr, "%d\n", rm_dmaxlen );
	}

	if( rm_dopt || rm_hopt )
		RM_dump( stderr, rm_dopt, rm_dopt, rm_dopt, rm_hopt );

	if( rm_dopt || rm_popt )
		SC_dump( stderr );

	if( rm_error )
		exit( 1 );

	if( rm_copt )
		exit( 0 );

	ip = RM_find_id( "chk_both_strs" );
	if( ip == NULL ){
		chk_both_strs = 1;
	}else
		chk_both_strs = ip->i_val.v_value.v_ival;

#ifdef	USE_GENBANK
	if( rm_dtype == DT_GENBANK ){
		if( ( dbp = GB_opendb( rm_dbfp ) ) == NULL )
			exit( 1 );
	}
#endif

	for( ; ; ){
		if( rm_dtype == DT_FASTN )
			slen = FN_fgetseq( rm_dbfp, sid, sdef, SBUF_SIZE, sbuf );
#ifdef	USE_GENBANK
		else
			slen = GB_fgetseq( dbp, sid, SBUF_SIZE, sbuf );
#endif
		if( slen == 0 )
			break;

		find_motif_driver( rm_n_searches, rm_searches, rm_sites,
			sid, rm_dtype, sdef, 0, slen, sbuf );
		if( chk_both_strs ){
			mk_rcmp( slen, sbuf, csbuf );
			find_motif_driver( rm_n_searches, rm_searches, rm_sites,
				sid, rm_dtype, sdef, 1, slen, csbuf );
		}
	}

	exit( 0 );
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
