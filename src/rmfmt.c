#include <stdio.h>
#include <string.h>

#define	MAXFIELDS	200
#define	FIELD1		4

main( argc, argv )
int	argc;
char	*argv[];
{
	FILE	*ifp, *tfp1, *tfp2;
	char	*tfnp1, *tfnp2;
	char	cmd[ 256 ];
	char	line[ 1024 ];
	char	*fields[ MAXFIELDS ];
	int	f, n_fields, t_fields;
	int	*maxw, fw, fs;

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

	tfnp1 = tempnam( NULL, "raw" );
	if( ( tfp1 = fopen( tfnp1, "w" ) ) == NULL ){
		fprintf( stderr, "%s: can't open temp-file '%s'\n",
			argv[ 0 ], tfnp1 );
		exit( 1 );
	}

	for( t_fields = 0; fgets( line, sizeof( line ), ifp ); ){
		n_fields = split( line, fields, " \t\n" );
		if( t_fields == 0 ){
			t_fields = n_fields;
			maxw = ( int * )
				malloc( ( t_fields - FIELD1 ) * sizeof( int ) );
			for( f = 0; f < t_fields - FIELD1; f++ )
				maxw[ f ] = 0;
		}
		for( f = 0; f < t_fields - FIELD1; f++ ){
			fw = strlen( fields[ FIELD1 + f ] );
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
	sprintf( cmd, "sort +0 -1 +1n -2 +2n -3 +3n -4 %s > %s\n",
		tfnp1, tfnp2 );
	system( cmd );

	if( ( tfp2 = fopen( tfnp2, "r" ) ) == NULL ){
		fprintf( stderr,
			"%s: can't open sorted temp-file '%s'\n",
			argv[ 0 ], tfnp2 );
		exit( 1 );
	}

	while( fgets( line, sizeof( line ), tfp2 ) ){
		n_fields = split( line, fields, " \t\n" );
		printf( "%-12s %s %7s %4s",
			fields[ 0 ], fields[ 1 ], fields[ 2 ], fields[ 3 ] );
		for( f = 0; f < t_fields - FIELD1; f++ ){
			fw = strlen( fields[ FIELD1 + f ] );
			printf( " %s", fields[ FIELD1 + f ] );
			for( fs = 0; fs < maxw[ f ] - fw; fs++ )
				putchar( ' ' );
		}
		printf( "\n" );
		for( f = 0; f < n_fields; f++ )
			free( fields[ f ] );
	}
	fclose( tfp2 );

/*
	unlink( tfnp1 );
	unlink( tfnp2 );
*/

	if( ifp != stdin )
		fclose( ifp );

	exit( 0 );
}
