#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "log.h"
#include "rmdefs.h"
#include "rnamot.h"
#include "dbutil.h"

extern	FILE	*yyin;

extern	int	rm_error;

extern	ARGS_T	*rm_args;
extern	int	rm_preprocess;
extern	int	rm_unlink_xdf;
extern	FILE	*rm_dbfp;

extern	STREL_T	rm_descr[];
extern	int	rm_n_descr;
extern	int	rm_dminlen;	/* min. len. of entire motif	*/
extern	int	rm_dmaxlen;	/* max. len. of entire motif	*/

extern	SITE_T	*rm_sites;

extern	SEARCH_T	**rm_searches;
extern	int		rm_n_searches;

static	char	sid[ SID_SIZE ];
static	char	sdef[ SDEF_SIZE ];
static	int	s_sbuf;
static	char	*sbuf;
static	int	slen;

static	void	mk_rcmp( int, char [] );

int
main( int argc, char *argv[] )
{
	IDENT_T	*ip;
	int	done = 0;
	int	chk_both_strs;
	int	show_progress;
	int	ecnt;
	int	( *fgetseq )( FILE *, char *, int, char *, int, char * );

	if( RM_init( argc, argv ) )
		exit( 1 );

	if( rm_args->a_vopt ){
		fprintf( stderr, "%s: %s.\n", argv[ 0 ], VERSION );
		done = 1;
	}
	if( rm_args->a_sopt ){
		RM_dump( stderr, 2, 0, 0, 0 );
		done = 1;
	}
	if( done )
		exit( 0 );

	if( rm_preprocess ){
		if( ( rm_args->a_xdfname = RM_preprocessor() ) == NULL )
			exit( 1 );
	}

	if( ( yyin = fopen( rm_args->a_xdfname, "r" ) ) == NULL ){
		LOG_ERROR("can't read xd-file %s.", rm_args->a_xdfname );
		exit( 1 );
	}

	if( yyparse() ){
		RM_errormsg( 0, "syntax error." );
	}

	if( rm_unlink_xdf )
		unlink( rm_args->a_xdfname );

	if( !rm_error ){
		if( SE_link( rm_n_descr, rm_descr ) )
			exit( 1 );
		RM_linkscore();
		if( rm_args->a_dfname != NULL ){
			fprintf( stderr,
				"%s: complete descr length: min/max = %d/",
				rm_args->a_dfname, rm_dminlen );
			if( rm_dmaxlen == UNBOUNDED )
				fprintf( stderr, "UNBND\n" );
			else
				fprintf( stderr, "%d\n", rm_dmaxlen );
		}
	}

	if( rm_args->a_dopt || rm_args->a_hopt )
		RM_dump( stderr, rm_args->a_dopt, rm_args->a_dopt,
			rm_args->a_dopt, rm_args->a_hopt );

	if( rm_args->a_dopt || rm_args->a_popt )
		RM_dumpscore( stderr );

	if( rm_error )
		exit( 1 );

	if( rm_args->a_copt )
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

	if( rm_args->a_dbfmt == NULL )
		fgetseq = FN_fgetseq;
	else if( !strcmp( rm_args->a_dbfmt, DT_FASTN ) )
		fgetseq = FN_fgetseq;
	else if( !strcmp( rm_args->a_dbfmt, DT_PIR ) )
		fgetseq = PIR_fgetseq;
	else if( !strcmp( rm_args->a_dbfmt, DT_GENBANK ) )
		fgetseq = GB_fgetseq;
	else{
		LOG_ERROR("unknown data format %s.", rm_args->a_dbfmt );
		exit( 1 );
	}
	rm_dbfp = DB_fnext( rm_dbfp, &rm_args->a_c_dbfname,
		rm_args->a_n_dbfname, rm_args->a_dbfname );
	if( rm_dbfp == NULL )
		exit( 1 );

	s_sbuf = rm_args->a_maxslen;
	sbuf = ( char * )malloc( s_sbuf * sizeof( char ) );
	if( sbuf == NULL ){
		LOG_ERROR("can't allocate sbuf (s_sbuf=%d)", s_sbuf );
		exit( 1 );
	}
	
	if( RM_fm_init() ){
		exit( 1 );
	}

	RM_setprog( P_BEGIN );
	RM_score( 0, 0, NULL, NULL );

	RM_setprog( P_MAIN );
	for( ecnt = 0; ; ){
		slen = fgetseq( rm_dbfp, sid,
			SDEF_SIZE, sdef, s_sbuf, sbuf );
		if( slen == EOF ){
			rm_dbfp = DB_fnext( rm_dbfp,
				&rm_args->a_c_dbfname, rm_args->a_n_dbfname,
				rm_args->a_dbfname );
			if( rm_dbfp == NULL )
				break;
		}

		ecnt++;

		if( show_progress ){
			if( ecnt % show_progress == 0 )
				fprintf( stderr, "%s: %7d: %s\n",
					argv[ 0 ], ecnt, sid );
		}

		RM_find_motif( rm_n_searches, rm_searches, rm_sites,
			sid, sdef, 0, slen, sbuf );
		if( chk_both_strs ){
			mk_rcmp( slen, sbuf );
			RM_find_motif( rm_n_searches, rm_searches, rm_sites,
				sid, sdef, 1, slen, sbuf );
		}
	}

	RM_setprog( P_END );
	RM_score( 0, 0, NULL, NULL );

	exit( 0 );
}

static	void	mk_rcmp( int slen, char sbuf[] )
{
	static	int	init = 0;
	static	char	wc_cmp[ 128 ];
	char	*sp, *cp;
	int	i, c;

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

	for( sp = sbuf, cp = &sbuf[ slen - 1 ]; sp <= cp; sp++, cp-- ){
		c = wc_cmp[ (int)*sp ];
		*sp = wc_cmp[ (int)*cp ];
		*cp = c;
	}
}
