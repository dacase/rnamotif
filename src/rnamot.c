#include <stdio.h>

#include "rnamot.h"

extern	FILE	*yyin;

extern	int	rm_error;
extern	char	*rm_dfname;
extern	char	*rm_xdfname;
extern	int	rm_preprocess;
extern	int	rm_unlink_xdf;
extern	int	rm_copt;
extern	int	rm_dopt;
extern	int	rm_hopt;
extern	int	rm_popt;
extern	int	rm_sopt;
extern	int	rm_vopt;
extern	FILE	*rm_dbfp;
extern	char	**rm_dbfname;
extern	int	rm_n_dbfname;
extern	int	rm_c_dbfname;

extern	STREL_T	rm_descr[];
extern	int	rm_n_descr;
extern	int	rm_dminlen;	/* min. len. of entire motif	*/
extern	int	rm_dmaxlen;	/* max. len. of entire motif	*/

extern	SITE_T	*rm_sites;

extern	SEARCH_T	**rm_searches;
extern	int		rm_n_searches;

#define	SID_SIZE	100
static	char	sid[ SID_SIZE ];
#define	SDEF_SIZE	20000
static	char	sdef[ SDEF_SIZE ];
#define	SBUF_SIZE	30000000
static	char	sbuf[ SBUF_SIZE ];
static	int	slen;
static	char	csbuf[ SBUF_SIZE ];

IDENT_T	*RM_find_id();

char	*RM_preprocessor( void );
FILE	*FN_fnext( FILE *, int *, int, char *[] );

static	void	mk_rcmp( int, char [], char [] );

main( int argc, char *argv[] )
{
	IDENT_T	*ip;
	int	done = 0;
	int	chk_both_strs;
	int	show_progress;
	int	ecnt;

	if( RM_init( argc, argv ) )
		exit( 1 );

	if( rm_vopt ){
		fprintf( stderr, "%s: %s.\n", argv[ 0 ], VERSION );
		done = 1;
	}
	if( rm_sopt ){
		RM_dump( stderr, 2, 0, 0, 0 );
		done = 1;
	}
	if( done )
		exit( 0 );

	if( rm_preprocess ){
		if( ( rm_xdfname = RM_preprocessor() ) == NULL )
			exit( 1 );
	}

	if( ( yyin = fopen( rm_xdfname, "r" ) ) == NULL ){
		fprintf( stderr, "%s: can't read xd-file %s.\n",
			argv[ 0 ], rm_xdfname );
		exit( 1 );
	}

	if( yyparse() ){
		RM_errormsg( 0, "syntax error." );
	}

	if( rm_unlink_xdf )
		unlink( rm_xdfname );

	if( !rm_error ){
		if( SE_link( rm_n_descr, rm_descr ) )
			exit( 1 );
		RM_linkscore();
		if( rm_dfname != NULL ){
			fprintf( stderr,
				"%s: complete descr length: min/max = %d/",
				rm_dfname, rm_dminlen );
			if( rm_dmaxlen == UNBOUNDED )
				fprintf( stderr, "UNBND\n" );
			else
				fprintf( stderr, "%d\n", rm_dmaxlen );
		}
	}

	if( rm_dopt || rm_hopt )
		RM_dump( stderr, rm_dopt, rm_dopt, rm_dopt, rm_hopt );

	if( rm_dopt || rm_popt )
		RM_dumpscore( stderr );

	if( rm_error )
		exit( 1 );

	if( rm_copt )
		exit( 0 );

	ip = RM_find_id( "chk_both_strs" );
	if( ip == NULL )
		chk_both_strs = 1;
	else
		chk_both_strs = ip->i_val.v_value.v_ival;

	ip = RM_find_id( "show_progress" );
	if( ip == NULL )
		show_progress = 0;
	else
		show_progress = ip->i_val.v_value.v_ival;

	rm_dbfp = FN_fnext( rm_dbfp, &rm_c_dbfname, rm_n_dbfname, rm_dbfname );
	if( rm_dbfp == NULL )
		exit( 1 );

	for( ecnt = 0; ; ){
		slen = FN_fgetseq( rm_dbfp, sid,
			SDEF_SIZE, sdef, SBUF_SIZE, sbuf );
		if( slen == EOF ){
			rm_dbfp = FN_fnext( rm_dbfp,
				&rm_c_dbfname, rm_n_dbfname, rm_dbfname );
			if( rm_dbfp == NULL )
				break;
		}

		ecnt++;

		if( show_progress ){
			if( ecnt % show_progress == 0 )
				fprintf( stderr, "%s: %7d: %s\n",
					argv[ 0 ], ecnt, sid );
		}

		find_motif_driver( rm_n_searches, rm_searches, rm_sites,
			sid, sdef, 0, slen, sbuf );
		if( chk_both_strs ){
			mk_rcmp( slen, sbuf, csbuf );
			find_motif_driver( rm_n_searches, rm_searches, rm_sites,
				sid, sdef, 1, slen, csbuf );
		}
	}

	exit( 0 );
}

static	void	mk_rcmp( int slen, char sbuf[], char csbuf[] )
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
