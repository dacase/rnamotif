#include <stdio.h>
#include <string.h>

#define	U_MSG_S	"usage: %s [ -l ] [ rm-output-file ]\n"

#define	MAXFIELDS	200
#define	FIELD1		4
#define	MAXW		20
static	char	*fields[ MAXFIELDS ];
static	int	n_fields, t_fields;

static	int	*maxw;

#define	FMT_LEFT	0
#define	FMT_RIGHT	1
static	int	*fmt;
static	int	*getfmt();
static	int	fcmprs();

main( argc, argv )
int	argc;
char	*argv[];
{
	char	*ifname;
	FILE	*ifp, *tfp1, *tfp2;
	char	*tfnp1, *tfnp2;
	char	cmd[ 256 ];
	char	line[ 10000 ];
	char	*sp, *vbp;
	int	ac, field1, f, fw, fs;
	int	lopt, scored;

/*
	if( argc == 1 )
		ifp = stdin;
	else if( argc > 2 ){
		fprintf( stderr, "usage: %s [ rm-output-file ]\n", argv[ 0 ] );
		exit( 1 );
	}else if( ( ifp = fopen( argv[ 1 ], "r" ) ) == NULL ){
		fprintf( stderr, "%s: can't open rm-output-file '%s'\n",
			argv[ 0 ], argv[ 1 ] );
		exit( 1 );
	}
*/
	lopt = 0;
	ifname = NULL;
	for( ac = 1; ac < argc; ac++ ){
		if( !strcmp( argv[ ac ], "-l" ) )
			lopt = 1;
		else if( *argv[ ac ] == '-' ){
			fprintf( stderr, U_MSG_S, argv[ 0 ] );
			exit( 1 );
		}else if( ifname != NULL ){
			fprintf( stderr, U_MSG_S, argv[ 0 ] );
			exit( 1 );
		}else
			ifname = argv[ ac ];
	}
	if( ifname == NULL ){
		ifp = stdin;
	}else if( ( ifp = fopen( ifname, "r" ) ) == NULL ){
		fprintf( stderr, "%s: can't read rm-output-file '%s'\n",
			argv[ 0 ], ifname );
		exit( 1 );
	}

	tfnp1 = tempnam( NULL, "raw" );
	if( ( tfp1 = fopen( tfnp1, "w" ) ) == NULL ){
		fprintf( stderr, "%s: can't open temp-file '%s'\n",
			argv[ 0 ], tfnp1 );
		exit( 1 );
	}

	scored = 0;	/* #RM scored line seen */
	field1 = FIELD1;
	for( t_fields = 0; fgets( line, sizeof( line ), ifp ); ){
		/* skip fastn def lines	*/
		if( *line == '>' )
			continue;
		n_fields = split( line, fields, " \t\n" );
		if( n_fields == 0 )
			continue;
		else if( !strcmp( fields[ 0 ], "#RM" ) ){
			if( n_fields < 2 )
				continue;
			if( !strcmp( fields[ 1 ], "descr" ) ){
				fmt = getfmt( n_fields, field1, fields );
				if( fmt == NULL )
					exit( 1 );
			}else if( !strcmp( fields[ 1 ], "scored" ) ){
				field1++;
				scored = 1;
			}
			fputs( line, stdout );
			continue;
		}else if( *fields[ 0 ] == '#' )
			continue;
		if( t_fields == 0 ){
			t_fields = n_fields;
			maxw = ( int * )malloc( ( t_fields ) * sizeof( int ) );
			if( maxw == NULL ){
				fprintf( stderr, "%s: can't allocate maxw.\n",
					argv[ 0 ] );
				exit( 1 );
			}
			for( f = 0; f < t_fields; f++ )
				maxw[ f ] = 0;
		}
		if( lopt ){
			for( vbp = NULL, sp = fields[ 0 ]; *sp; sp++ ){
				if( *sp == '|' )
					vbp = sp;
			}
			if( vbp != NULL )
				strcpy( fields[ 0 ], &vbp[ 1 ] );
		}
		fw = strlen( fields[ 0 ] );
		if( fw > maxw[ 0 ] )
			maxw[ 0 ] = fw;
		for( f = 1; f < field1; f++ ){
			fw = strlen( fields[ f ] );
			if( fw > maxw[ f ] )
				maxw[ f ] = fw;
		}
		for( f = field1; f < t_fields; f++ ){
			fw = strlen( fields[ f ] );
			if( fw > MAXW )
				fw = fcmprs( fw, fields[ f ] );
			if( fw > maxw[ f ] )
				maxw[ f ] = fw;
		}
		fprintf( tfp1, "%s", fields[ 0 ] );
		for( f = 1; f < n_fields; f++ )
			fprintf( tfp1, " %s", fields[ f ] );
		fprintf( tfp1, "\n" );
		for( f = 0; f < n_fields; f++ )
			free( fields[ f ] );
	}
	fclose( tfp1 );

	tfnp2 = tempnam( NULL, "srt" );
	if( scored ){
		sprintf( cmd,
			"sort +1rn -2 +0 -1 +2n -3 +3n -4 +4n -5 %s > %s\n",
			tfnp1, tfnp2 );
	}else{
		sprintf( cmd, "sort +0 -1 +1n -2 +2n -3 +3n -4 %s > %s\n",
			tfnp1, tfnp2 );
	}
	system( cmd );

	if( ( tfp2 = fopen( tfnp2, "r" ) ) == NULL ){
		fprintf( stderr,
			"%s: can't open sorted temp-file '%s'\n",
			argv[ 0 ], tfnp2 );
		exit( 1 );
	}

	while( fgets( line, sizeof( line ), tfp2 ) ){
		n_fields = split( line, fields, " \t\n" );
		for( f = 0; f < t_fields; f++ ){
			fw = strlen( fields[ f ] );
			if( fmt[ f ] == FMT_LEFT ){
				if( f != 0 )
					putchar( ' ' );
				printf( "%s", fields[ f ] );
				for( fs = 0; fs < maxw[ f ] - fw; fs++ )
					putchar( ' ' );
			}else{
				printf( " " );
				for( fs = 0; fs < maxw[ f ] - fw; fs++ )
					putchar( ' ' );
				printf( "%s", fields[ f ] );
			}
		}
		printf( "\n" );
		for( f = 0; f < n_fields; f++ )
			free( fields[ f ] );
	}
	fclose( tfp2 );

	unlink( tfnp1 );
	unlink( tfnp2 );

	if( ifp != stdin )
		fclose( ifp );

	exit( 0 );
}

static	int	*getfmt( n_fields, field1, fields )
int	n_fields;
int	field1;
char	*fields[];
{
	int	f;
	int	*fmt;

	fmt = ( int * )malloc( ( n_fields - 2 + field1 ) * sizeof( int ) );
	if( fmt == NULL ){
		fprintf( stderr, "getfmt: can't allocate fmt.\n" );
		return( NULL );
	}
	fmt[ 0 ] = FMT_LEFT;
	for( f = 1; f < field1; f++ )
		fmt[ f ] = FMT_RIGHT;
	for( f = 2; f < n_fields; f++ ){
		if( !strncmp( fields[ f ], "h3", 2 ) ||
				!strncmp( fields[ f ], "t2", 2 ) ||
				!strncmp( fields[ f ], "q2", 2 ) ||
				!strncmp( fields[ f ], "q4", 2 ) )
			fmt[ f - 2 + field1 ] = FMT_RIGHT;
		else
			fmt[ f - 2 + field1 ] = FMT_LEFT;
	}
	return( fmt );
}

static	int	fcmprs( flen, field )
int	flen;
char	field[];
{
	char	tmp[ 10000 ];

	if( flen > MAXW ){
		sprintf( tmp, "%.*s...(%d)...%.*s",
			3, field, flen, 3, &field[ flen - 3 ] );
		strcpy( field, tmp );
		return( strlen( field ) );
	}else
		return( flen );
}
