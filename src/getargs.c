#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "rmdefs.h"
#include "rnamot.h"

double	atof();

ARGS_T	*RM_getargs( int argc, char *argv[] )
{
	ARGS_T	*argp = NULL;
	int	ac, err, len, cldsize;
	char	*sp;
	INCDIR_T	*id1, *idt;

	argp = ( ARGS_T * )calloc( 1L, sizeof( ARGS_T ) );
	if( argp == NULL ){
		fprintf( stderr, "RM_getargs: can't allocate rm_args.\n" );
		return( NULL );
	}
	argp->a_o_emin = 2.5;

	argp->a_dbfname = ( char ** )malloc( argc * sizeof( char * ) );
	if( argp->a_dbfname == NULL ){
		fprintf( stderr, "RM_getargs: can't allocate a_dbfname.\n" );
		return( NULL );
	}
	argp->a_n_dbfname = 0;
	argp->a_c_dbfname = UNDEF;

	id1 = idt = NULL;	/* list of include dirs */
	for( err = FALSE, cldsize = 0, ac = 1; ac < argc; ac++ ){
		if( !strcmp( argv[ ac ], "-c" ) )
			argp->a_copt = TRUE;
		else if( !strcmp( argv[ ac ], "-d" ) )
			argp->a_dopt = TRUE;
		else if( !strcmp( argv[ ac ], "-h" ) )
			argp->a_hopt = TRUE;
		else if( !strcmp( argv[ ac ], "-N" ) ){
			if( ac == argc - 1 ){
				fprintf( stderr, U_MSG_S, argv[ 0 ] );
				err = TRUE;
				break;
			}else if( argp->a_maxslen != 0 ){
				fprintf( stderr, U_MSG_S, argv[ 0 ] );
				err = TRUE;
				break;
			}else{
				ac++;
				argp->a_maxslen = atoi( argv[ ac ] ) + 1;

fprintf( stderr, "getargs: set maxslen to %d\n", argp->a_maxslen );

			}
		}else if( !strncmp( argv[ ac ], "-O", 2 ) ){
			sp = &argv[ ac ][ 2 ];
			if( *sp == '\0' ){
				fprintf( stderr, U_MSG_S, argv[ 0 ] );
				err = TRUE;
				break;
			}else{
				argp->a_o_emin = atof( sp );
				if( argp->a_o_emin < 0.25 )
					argp->a_o_emin = 0.0;
			}
		}else if( !strcmp( argv[ ac ], "-p" ) )
			argp->a_popt = TRUE;
		else if( !strcmp( argv[ ac ], "-s" ) )
			argp->a_sopt = TRUE;
		else if( !strcmp( argv[ ac ], "-v" ) )
			argp->a_vopt = TRUE;
		else if( !strcmp( argv[ ac ], "-context" ) )
			argp->a_show_context = TRUE;
		else if( !strcmp( argv[ ac ], "-sh" ) )
			argp->a_strict_helices = TRUE;
		else if( !strcmp( argv[ ac ], "-descr" ) ){
			if( ac == argc - 1 ){
				fprintf( stderr, U_MSG_S, argv[ 0 ] );
				err = TRUE;
				break;
			}else if( argp->a_dfname != NULL ){
				fprintf( stderr, U_MSG_S, argv[ 0 ] );
				err = TRUE;
				break;
			}else{
				ac++;
				argp->a_dfname = argv[ ac ];
			}
		}else if( !strcmp( argv[ ac ], "-xdescr" ) ){
			if( ac == argc - 1 ){
				fprintf( stderr, U_MSG_S, argv[ 0 ] );
				err = TRUE;
				break;
			}else if( argp->a_xdfname != NULL ){
				fprintf( stderr, U_MSG_S, argv[ 0 ] );
				err = TRUE;
				break;
			}else{
				ac++;
				argp->a_xdfname = argv[ ac ];
			}
		}else if( !strncmp( argv[ ac ], "-D", 2 ) ){
			cldsize += strlen( &argv[ ac ][ 2 ] ) + 2;
		}else if( !strncmp( argv[ ac ], "-I", 2 ) ){
			id1 = ( INCDIR_T * )malloc( sizeof( INCDIR_T ) );
			if( id1 == NULL ){
				fprintf( stderr,
			"RM_getargs: can't allocate space for incdir %s.",
					argv[ ac ] );
				err = TRUE;
				break;
			}
			len = strlen( &argv[ ac ][ 2 ] ) + 1;
			sp = ( char * ) malloc( len * sizeof( char ) );
			if( sp == NULL ){
				fprintf( stderr,
				"RM_getargs: can't allocate sp for incdir %s.",
					argv[ ac ] );
				err = TRUE;
				break;
			}
			strcpy( sp, &argv[ ac ][ 2 ] );
			id1->i_next = NULL;
			id1->i_name = sp;
			if( argp->a_idlist == NULL )
				argp->a_idlist = id1;
			else
				idt->i_next = id1;
			idt = id1;
		}else if( !strcmp( argv[ ac ], "-xdfname" ) ){
			if( ac == argc - 1 ){
				fprintf( stderr, U_MSG_S, argv[ 0 ] );
				err = TRUE;
				break;
			}else if( argp->a_xdfname != NULL ){
				fprintf( stderr, U_MSG_S, argv[ 0 ] );
				err = TRUE;
				break;
			}else{
				ac++;
				argp->a_xdfname = argv[ ac ];
			}
		}else if( !strcmp( argv[ ac ], "-fmt" ) ){
			if( ac == argc - 1 ){
				fprintf( stderr, U_MSG_S, argv[ 0 ] );
				err = TRUE;
				break;
			}
			ac++;
			if( !strcmp( argv[ ac ], DT_FASTN ) )
				argp->a_dbfmt = DT_FASTN;
			else if( !strcmp( argv[ ac ], DT_PIR ) )
				argp->a_dbfmt = DT_PIR;
			else if( !strcmp( argv[ ac ], DT_GENBANK ) )
				argp->a_dbfmt = DT_GENBANK;
			else{
				fprintf( stderr, U_MSG_S, argv[ 0 ] );
				err = TRUE;
				break;
			}
		}else if( !strcmp( argv[ ac ], "-fmap" ) ){
			if( ac == argc - 1 ){
				fprintf( stderr, U_MSG_S, argv[ 0 ] );
				err = TRUE;
				break;
			}else if( argp->a_fmfname != NULL ){
				fprintf( stderr, U_MSG_S, argv[ 0 ] );
				err = TRUE;
				break;
			}else{
				ac++;
				argp->a_fmfname = argv[ ac ];
			}
		}else if( *argv[ ac ] == '-' ){
			fprintf( stderr, U_MSG_S, argv[ 0 ] );
			err = TRUE;
			break;
		}else{
			argp->a_dbfname[ argp->a_n_dbfname ] = argv[ ac ];
			argp->a_n_dbfname++;
		}
	}

	if( err )
		return( NULL );

	if( argp->a_maxslen == 0 )
		argp->a_maxslen = MAXSLEN + 1;

	if( cldsize > 0 ){
		cldsize++;	/* add space for '\0'	*/
		argp->a_cldefs = ( char * )malloc( cldsize * sizeof( char ) );
		if( argp->a_cldefs == NULL ){
			fprintf( stderr,
		"RM_getargs: can't allocate space for cmd-line defs.\n" );
			return( NULL );
		}
		for( sp = argp->a_cldefs, ac = 1; ac < argc; ac++ ){
			if( !strncmp( argv[ ac ], "-D", 2 ) ){ 
				strcpy( sp, &argv[ ac ][ 2 ] );
				sp += strlen( sp );
				*sp++ =  ';';
				*sp++ =  ' ';
			}
		}
		sp[-1] = '\n';
		*sp = '\0';
	}

	return( argp );
}
