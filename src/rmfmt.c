#include <stdio.h>
#include <string.h>

#define	MAXFIELDS	200
#define	FIELD1		4

main( argc, argv )
int	argc;
char	*argv[];
{
	FILE	*ifp, *tfp;
	char	*tfnp;
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

	tfnp = tmpnam( NULL );
	if( ( tfp = fopen( tfnp, "w+" ) ) == NULL ){
		fprintf( stderr, "%s: can't open temp-file '%s'\n",
			argv[ 0 ], tfnp );
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
		fprintf( tfp, "%s", fields[ 0 ] );
		for( f = 1; f < n_fields; f++ )
			fprintf( tfp, " %s", fields[ f ] );
		fprintf( tfp, "\n" );
		for( f = 0; f < n_fields; f++ )
			free( fields[ f ] );
	}

	rewind( tfp );

	while( fgets( line, sizeof( line ), tfp ) ){
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

	fclose( tfp );
	unlink( tfnp );

	if( ifp != stdin )
		fclose( ifp );

	exit( 0 );
}
