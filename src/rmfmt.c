#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#define	U_MSG_S	\
"usage: %s [ -a ] [ -l[a] ] [ -smax N ] [ -td dir ] [ rm-output-file ]\n"

#define	MIN(a,b)	((a)<(b)?(a):(b))
#define	MAX(a,b)	((a)>(b)?(a):(b))

#define	L_OFF		0
#define	L_LOCUS		1
#define	L_ACC		2

#define	SMAX		30000000	/* largest file to sort */
#define	MAXFIELDS	200
#define	OFIELD1		1
#define	DFIELD1		4
#define	MAXW		20
static	char	*fields[ MAXFIELDS ];
static	char	*sfields[ MAXFIELDS ];
static	int	smaxw;
static	char	*ofields[ MAXFIELDS ];
static	int	*ofmt;
static	int	*omaxw;
static	int	n_fields, n_ofields, n_sfields, n_sfields1, m_sfields;

#define	LINE_SIZE	50000
static	char	ra_line[ LINE_SIZE ];
static	char	line[ LINE_SIZE ];
static	char	dline[ LINE_SIZE ];

#define	WBSIZE	70

#define	FMT_LEFT	0
#define	FMT_RIGHT	1
static	int	getline( char [], FILE * );
static	void	ungetline( char [] );
static	int	*getfmt( int, int, char *[], int * );
static	int	is_a_number( char [] );
static	int	fcmprs( int, char [] );
static	void	align( FILE *, int, char [], char [] );

main( int argc, char *argv[] )
{
	char	*ifname, *tfdir;
	FILE	*ifp, *tfp1, *tfp2;
/*
	char	*tfnp1, *tfnp2;
*/
	char	tfnp1[ 256 ], tfnp2[ 256 ];
	int	tfd1, tfd2;
	char	cmd[ 256 ];
	char	*sp, *vbp, *l_vbp;
	int	ac, f, fw, fs, of1, df1;
	int	aopt, lopt, scored, sort;
	int	smax;
	struct	stat	sbuf;

	ifname = NULL;
	tfdir = NULL;
	aopt = 0;	/* 1 = make fastn alignment		*/ 
	lopt = L_OFF;	/* 1 = print only entry's locus name	*/
			/* 2 = print only entry's acc#		*/
	smax = SMAX;	/* Don't sort files larger than SMAX	*/
	for( ac = 1; ac < argc; ac++ ){
		if( !strcmp( argv[ ac ], "-a" ) )
			aopt = 1;
		else if( !strcmp( argv[ ac ], "-l" ) )
			lopt = L_LOCUS;
		else if( !strcmp( argv[ ac ], "-la" ) )
			lopt = L_ACC;
		else if( !strcmp( argv[ ac ], "-smax" ) ){
			ac++;
			if( ac >= argc ){
				fprintf( stderr, U_MSG_S, argv[ 0 ] );
				exit( 1 );
			}
			smax = atoi( argv[ ac ] );
		}else if( !strcmp( argv[ ac ], "-td" ) ){
			ac++;
			if( ac >= argc ){
				fprintf( stderr, U_MSG_S, argv[ 0 ] );
				exit( 1 );
			}else if( tfdir != NULL ){
				fprintf( stderr, U_MSG_S, argv[ 0 ] );
				exit( 1 );
			}else
				tfdir = argv[ ac ];
		}else if( *argv[ ac ] == '-' ){
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
	if( tfdir == NULL )
		tfdir = P_tmpdir;

/*
	tfnp1 = tempnam( tfdir, "raw" );
	if( ( tfp1 = fopen( tfnp1, "w+" ) ) == NULL ){
		fprintf( stderr, "%s: can't open temp-file '%s'\n",
			argv[ 0 ], tfnp1 );
		exit( 1 );
	}
*/
	sprintf( tfnp1, "%s/raw_XXXXXX", tfdir );
	tfd1 = mkstemp( tfnp1 );
	if( ( tfp1 = fdopen( tfd1, "w+" ) ) == NULL ){
		fprintf( stderr, "%s: can't open temp-file '%s'\n",
			argv[ 0 ], tfnp1 );
		exit( 1 );
	}

	while( getline( line, ifp ) ){
		if( *line == '>' ){
			ungetline( line );
			break;
		}
		n_fields = split( line, fields, " \t\n" );
		if( n_fields == 0 )
			continue;
		else if( !strcmp( fields[ 0 ], "#RM" ) ){
			if( n_fields < 2 )
				continue;
			if( !strcmp( fields[ 1 ], "descr" ) ){
				ofmt = getfmt( n_fields, DFIELD1, fields,
					&n_ofields );
				if( ofmt == NULL )
					exit( 1 );
			}
			if( !aopt )
				fputs( line, stdout );
			continue;
		}
		for( f = 0; f < n_fields; f++ )
			free( fields[ f ] );
	}

	omaxw = ( int * )malloc( ( n_ofields ) * sizeof( int ) );
	if( omaxw == NULL ){
		fprintf( stderr, "%s: can't allocate omaxw.\n",
			argv[ 0 ] );
		exit( 1 );
	}
	for( f = 0; f < n_ofields; f++ )
		omaxw[ f ] = 0;

	m_sfields = 0;
	for( n_sfields1 = 0, scored = 1, sort = 1; getline( line, ifp ); ){
		if( *line == '#' )
			continue;	/* allow cat of rnamotif runs */
		if( *line == '>' ){
			if( aopt )
				fputs( line, tfp1 );
			continue;
		}

		n_fields = split( line, fields, " \t\n" );
		df1 = n_fields - ( n_ofields - DFIELD1 );
		of1 = df1 - ( DFIELD1 - OFIELD1 );
		n_sfields = df1 - DFIELD1;
		m_sfields = MAX( m_sfields, n_sfields );

		if( n_sfields > 1 )
			scored = 0;

		if( n_sfields1 == 0 )
			n_sfields1 = n_sfields;
		else if( n_sfields != m_sfields )
			sort = 0;

		ofields[ 0 ] = fields[ 0 ];
		for( f = of1; f < n_fields; f++ )
			ofields[ f - of1 + 1 ] = fields[ f ];
		for( f = 0; f < n_sfields; f++ )
			sfields[ f ] = fields[ f + 1 ];

		if( scored )
			scored = is_a_number( sfields[ 0 ] );

		if( lopt ){
			for( l_vbp = vbp = NULL, sp = ofields[ 0 ]; *sp; sp++ ){
				if( *sp == '|' ){
					l_vbp = vbp;
					vbp = sp;
				}
			}
		}
		if( vbp != NULL ){
			if( lopt == L_LOCUS ){
				vbp++;
				if( *vbp != '\0' )
					strcpy( ofields[ 0 ], vbp );
				else if( l_vbp != NULL ){
					l_vbp++;
					vbp--;
					strncpy( ofields[0], l_vbp, vbp-l_vbp );
					ofields[0][vbp-l_vbp] = '\0';
				}else{
					vbp--;
					*vbp = '\0';
				}
			}else if( lopt == L_ACC ){
				if( l_vbp != NULL ){
					l_vbp++;
					strncpy( ofields[0], l_vbp, vbp-l_vbp );
					ofields[0][vbp-l_vbp] = '\0';
				}
			}
		}
		fw = strlen( ofields[ 0 ] );
		if( fw > omaxw[ 0 ] )
			omaxw[ 0 ] = fw;
		for( fw = 0, f = 0; f < n_sfields; f++ ){
			fw += strlen( sfields[ f ] );
		}
		fw += ( n_sfields - 1 );
		if( fw > smaxw )
			smaxw = fw;
		for( f = OFIELD1; f < DFIELD1; f++ ){
			fw = strlen( ofields[ f ] );
			if( fw > omaxw[ f ] )
				omaxw[ f ] = fw;
		}
		for( f = DFIELD1; f < n_ofields; f++ ){
			fw = strlen( ofields[ f ] );
			if( !aopt ){
				if( fw > MAXW )
					fw = fcmprs( fw, ofields[ f ] );
			}
			if( fw > omaxw[ f ] )
				omaxw[ f ] = fw;
		}
		fprintf( tfp1, "%s", ofields[ 0 ] );
		for( f = 0; f < n_sfields; f++ ) 
			fprintf( tfp1, " %s", sfields[ f ] );
		for( f = OFIELD1; f < n_ofields; f++ )
			fprintf( tfp1, " %s", ofields[ f ] );
		fprintf( tfp1, "\n" );
		for( f = 0; f < n_fields; f++ )
			free( fields[ f ] );
	}

	if( !aopt ){
		fclose( tfp1 );

		if( stat( tfnp1, &sbuf ) ){
			fprintf( stderr,
				"%s: can't stat raw temp-file '%s'\n",
				argv[ 0 ], tfnp1 );
			exit( 1 );
		}
		if( sbuf.st_size > smax )
			scored = sort = 0;

/*
		tfnp2 = tempnam( tfdir, "srt" );
*/
		sprintf( tfnp2, "%s/srt_XXXXXX", tfdir );
		tfd2 = mkstemp( tfnp2 );
		if( scored ){
			sprintf( cmd,
			"sort +1rn -2 +0 -1 +2n -3 +3n -4 +4n -5 %s > %s\n",
				tfnp1, tfnp2 );
		}else if( sort ){
			sprintf( cmd,
			"sort +0 -1 +%dn -%d +%dn -%d +%dn -%d %s > %s\n",
				n_sfields+1, n_sfields+2,
				n_sfields+2, n_sfields+3,
				n_sfields+3, n_sfields+4,
				tfnp1, tfnp2 );
		}else
			sprintf( cmd, "cp %s %s\n", tfnp1, tfnp2 );
		system( cmd );

/*
		if( ( tfp2 = fopen( tfnp2, "r" ) ) == NULL ){
			fprintf( stderr,
				"%s: can't open sorted temp-file '%s'\n",
				argv[ 0 ], tfnp2 );
			exit( 1 );
		}
*/
		if( ( tfp2 = fdopen( tfd2, "r" ) ) == NULL ){
			fprintf( stderr,
				"%s: can't open sorted temp-file '%s'\n",
				argv[ 0 ], tfnp2 );
			exit( 1 );
		}

		while( fgets( line, sizeof( line ), tfp2 ) ){
			n_fields = split( line, fields, " \t\n" );
			df1 = n_fields - ( n_ofields - DFIELD1 );
			of1 = df1 - ( DFIELD1 - OFIELD1 );
			n_sfields = df1 - DFIELD1;

			ofields[ 0 ] = fields[ 0 ];
			for( f = of1; f < n_fields; f++ )
				ofields[ f - of1 + 1 ] = fields[ f ];
			for( f = 0; f < n_sfields; f++ )
				sfields[ f ] = fields[ f + 1 ];

			for( f = 0; f < OFIELD1; f++ ){
				fw = strlen( ofields[ f ] );
				if( ofmt[ f ] == FMT_LEFT ){
					if( f != 0 )
						putchar( ' ' );
					printf( "%s", ofields[ f ] );
					for( fs = 0; fs < omaxw[f] - fw; fs++ )
						putchar( ' ' );
				}else{
					printf( " " );
					for( fs = 0; fs < omaxw[f] - fw; fs++ )
						putchar( ' ' );
					printf( "%s", ofields[ f ] );
				}
			}
			printf( " " );
			if( m_sfields == 1 ){
				fw = strlen( sfields[ 0 ] );
/*
				printf( " " );
*/
				for( fs = 0; fs < smaxw - fw; fs++ )
					putchar( ' ' );
				printf( "%s", sfields[ 0 ] );
			}else{
				for( fs = 0, f = 0; f < n_sfields; f++ ){
					fw = strlen( sfields[ f ] );
					if( f != 0 ){
						putchar( ' ' );
						fs++;
					}
					printf( "%s", sfields[ f ] );
					fs += fw;
				}
				for( ; fs < smaxw; fs++ )
					putchar( ' ' );
			}
			for( f = OFIELD1; f < n_ofields; f++ ){
				fw = strlen( ofields[ f ] );
				if( ofmt[ f ] == FMT_LEFT ){
					if( f != 0 )
						putchar( ' ' );
					printf( "%s", ofields[ f ] );
					for( fs = 0; fs < omaxw[f] - fw; fs++ )
						putchar( ' ' );
				}else{
					printf( " " );
					for( fs = 0; fs < omaxw[f] - fw; fs++ )
						putchar( ' ' );
					printf( "%s", ofields[ f ] );
				}
			}
			printf( "\n" );
			for( f = 0; f < n_fields; f++ )
				free( fields[ f ] );
		}
		fclose( tfp2 );
		unlink( tfnp2 );
	}else{
		rewind( tfp1 );
		*dline = *line = '\0';
		while( fgets( line, sizeof( line ), tfp1 ) ){
			if( *line == '>' )
				strcpy( dline, line );
			else{
				align( stdout, lopt, dline, line );
				*dline = *line = '\0';
			}
		}
		align( stdout, lopt, dline, line );
	}

	unlink( tfnp1 );

	if( ifp != stdin )
		fclose( ifp );

	exit( 0 );
}

static	int	getline( char line[], FILE *fp )
{
	int	i, c;
	char	*lp;

	if( *ra_line ){
		strcpy( line, ra_line );
		*ra_line = '\0';
		return( 1 );
	}
	for( lp = line, i = 0; ( c = getc( fp ) ) != EOF; i++ ){
		if( i < LINE_SIZE )
			*lp++ = c;
		if( c == '\n' )
			break;
	}
	*lp = '\0';
	return( lp > line );
}

static	void	ungetline( char line[] )
{

	strcpy( ra_line, line );
}

static	int	*getfmt( int n_fields, int df1, char *fields[], int *n_fmt )
{
	int	f;
	int	*fmt;

	*n_fmt = n_fields - 2 + df1;
	fmt = ( int * )malloc( *n_fmt * sizeof( int ) );
	if( fmt == NULL ){
		fprintf( stderr, "getfmt: can't allocate fmt.\n" );
		return( NULL );
	}
	fmt[ 0 ] = FMT_LEFT;
	for( f = 1; f < df1; f++ )
		fmt[ f ] = FMT_RIGHT;
	for( f = 2; f < n_fields; f++ ){
		if( !strncmp( fields[ f ], "h3", 2 ) ||
				!strncmp( fields[ f ], "t2", 2 ) ||
				!strncmp( fields[ f ], "q2", 2 ) ||
				!strncmp( fields[ f ], "q4", 2 ) )
			fmt[ f - 2 + df1 ] = FMT_RIGHT;
		else
			fmt[ f - 2 + df1 ] = FMT_LEFT;
	}

	return( fmt );
}

static	int	is_a_number( char str[] )
{
	char	*sp;
	int	mcnt, ecnt;
	int	efmt;

	sp = str;
	if( *sp == '-' )
		sp++;
	for( mcnt = efmt = ecnt = 0; isdigit( *sp ); sp++ )
		mcnt++;
	if( *sp == '.' )
		sp++;
	for( ; isdigit( *sp ); sp++ )
		mcnt++;
	if( *sp == 'e' || *sp == 'E' ){
		efmt = 1;
		sp++;
		if( *sp == '-' )
			sp++;
		for( ; isdigit( *sp ); sp++ )
			ecnt++;
	}
	if( mcnt == 0 ) 
		return( 0 );
	if( efmt && ecnt == 0 )
		return( 0 );
	if( *sp != '\0' )
		return( 0 );
	return( 1 );
}

static	int	fcmprs( int flen, char field[] )
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

static	void	align( FILE *fp, int lopt, char dline[], char line[] )
{
	char	*def;
	char	name[ 256 ], ver[ 10 ];
	static	char	l_name[ 256 ] = "";
	static	int	l_cnt = 1;
	int	f, fs, fw;
	int	df1, of1;
	char	*wp, work[ 50000 ];
	int	w, wb, wlim, wlen;

	if( !*dline || !*align )
		return;

	for( def = &dline[ 1 ]; *def && isspace( *def ); def++ )
		;
	if( *def ){
		def = strchr( def, ' ' );
		if( def ){
			for( ; isspace( *def ); def++ )
				;
		}
	}
	n_fields = split( line, fields, " \t\n" );
	df1 = n_fields - ( n_ofields - DFIELD1 );
	of1 = df1 - ( DFIELD1 - OFIELD1 );
	n_sfields = df1 - DFIELD1;

	ofields[ 0 ] = fields[ 0 ];
	for( f = of1; f < n_fields; f++ )
		ofields[ f - of1 + 1 ] = fields[ f ];
	for( f = 0; f < n_sfields; f++ )
		sfields[ f ] = fields[ f + 1 ];

	if( lopt ){
		sprintf( name, "%s_%s%s",
			ofields[0], ofields[2],
			*ofields[1] == '0' ? "d" : "c" );
	}else{
		sprintf( name, "%s_%s_%s_%s",
			ofields[0], ofields[1], ofields[2], ofields[3] );
	}
	if( !strcmp( l_name, name ) ){
		l_cnt++;
		sprintf( ver, "%s%d", !lopt ? "_" : "", l_cnt );
	}else{
		l_cnt = 1;
		*ver = '\0';
	}
	fprintf( fp, ">%s%s %s", name, ver, sfields[ 0 ] );
	for( f = 1; f < n_sfields; f++ )
		fprintf( fp, " %s", sfields[ f ] );
	fprintf( fp, " %s", def );
	strcpy( l_name, name );

	for( wp = work, f = DFIELD1; f < n_ofields; f++ ){
		if( f != DFIELD1 )
			*wp++ = '|';
		fw = strlen( ofields[ f ] );
		if( ofmt[ f ] == FMT_LEFT ){
			sprintf( wp, "%s", ofields[ f ] );
			wp += strlen( wp );
			for( fs = 0; fs < omaxw[ f ] - fw; fs++ )
				*wp++ = '-';
		}else{
			for( fs = 0; fs < omaxw[ f ] - fw; fs++ )
				*wp++ = '-';
			sprintf( wp, "%s", ofields[ f ] );
			wp += strlen( wp );
		}
	}
	*wp = '\0';
	wlen = wp - work;
	for( wp = work, wb = 0; wb < wlen; wb += WBSIZE ){
		wlim = MIN( wb + WBSIZE, wlen );
		for( w = wb; w < wlim; w++ )
			putc( *wp++, fp );
		putc( '\n', fp );
	}

	for( f = 0; f < n_fields; f++ )
		free( fields[ f ] );
}
