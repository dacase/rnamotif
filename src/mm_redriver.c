#include <stdio.h>
#include <string.h>

#define	U_MSG_S	"usage: %s -p pat [ -mm N ] [ file ]\n"
#define	UNBOUNDED	0x7fffffff

#define	EXPBUF_SIZE	256
char	expbuf[ EXPBUF_SIZE ];

extern	char	*loc1, *loc2;
extern	int	circf;

void	seqlen();
void	mm_dumppat();
int	mm_step();

main( argc, argv )
int	argc;
char	*argv[];
{
	int	ac, i, j, mm;
	int	minl, maxl, exact, mmok;
	char	msg[ 20 ];
	char	*pat, *fname;
	char	line[ 256 ], *elp;
	FILE	*fp;

	for( mm = 0, pat = NULL, fname = NULL, ac = 1; ac < argc; ac++ ){
		if( !strcmp( argv[ ac ], "-p" ) ){
			if( ac == argc - 1 ){
				fprintf( stderr, U_MSG_S, argv[ 0 ] );
				exit( 1 );
			}else if( pat != NULL ){
				fprintf( stderr, U_MSG_S, argv[ 0 ] );
				exit( 1 );
			}else{
				ac++;
				pat = argv[ ac ];
			}
		}else if( !strcmp( argv[ ac ], "-mm" ) ){
			if( ac == argc - 1 ){
				fprintf( stderr, U_MSG_S, argv[ 0 ] );
				exit( 1 );
			}else if( mm != 0 ){
				fprintf( stderr, U_MSG_S, argv[ 0 ] );
				exit( 1 );
			}else{
				ac++;
				mm = atoi( argv[ ac ] );
				if( mm < 0 ){
					fprintf( stderr,
						"%s: mm must be > 0.\n" );
					exit( 1 );
				}
			}
		}else if( *argv[ ac ] == '-' ){
			fprintf( stderr, U_MSG_S, argv[ 0 ] );
			exit( 1 );
		}else if( fname == NULL )
			fname = argv[ ac ];
		else{
			fprintf( stderr, U_MSG_S, argv[ 0 ] );
			exit( 1 );
		}
	}

	if( pat == NULL ){
		fprintf( stderr, U_MSG_S, argv[ 0 ] );
		exit( 1 );
	}

	if( fname == NULL )
		fp = stdin;
	else if( ( fp = fopen( fname, "r" ) ) == NULL ){
		fprintf( stderr, "%s: can't read file %s\n",
			argv[ 0 ], fname );
		exit( 1 );
	}

	for( i = 0; i < EXPBUF_SIZE; i++ )
		expbuf[ i ] = '\0';
	compile( pat, expbuf, &expbuf[ EXPBUF_SIZE ], '\0' );
	mm_dumppat( stdout, expbuf, &expbuf[ EXPBUF_SIZE ] );
	seqlen( expbuf, &minl, &maxl, &exact, &mmok );
	if( maxl == UNBOUNDED )
		strcpy( msg, "UNBND" );
	else
		sprintf( msg, "%5d", maxl );
	printf( "minl = %3d, maxl = %5s, exact = %d, mmok = %d\n",
		minl, msg, exact, mmok );

	while( fgets( line, sizeof( line ), fp ) ){
		elp = strchr( line, '\n' );
		*elp = '\0';
		printf( "Does %s ~ %s ?", line, pat );
		circf = *pat == '^';
		if( mmok ){
			if( mm_step( line, expbuf, mm ) ){
				printf( "  Yes (mm).\n" );
			}else
				printf( "  No (mm).\n" );
		}else if( step( line, expbuf ) )
			printf( "  Yes (std).\n" );
		else
			printf( "  No (std).\n" );

	}
	if( fp != stdin )
		fclose( fp );

	exit( 0 );
}
