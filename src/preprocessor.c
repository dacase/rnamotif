#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "rmdefs.h"
#include "rnamot.h"

extern	int	rm_error;

extern	ARGS_T	*rm_args;
extern	char	*rm_wdfname;
extern	int	rm_lineno;
extern	int	rm_emsg_lineno;
/*
extern	INCDIR_T	*rm_idlist;
*/

typedef	struct	fstk_t	{
	char	*f_fname;
	FILE	*f_fp;
	int	f_lineno;
} FSTK_T;

#define	FSTK_SIZE	10
static	FSTK_T	fstk[ FSTK_SIZE ];
static	int	fstkp = -1;
#define	IS_FSTK_EMPTY()	(fstkp==-1)
#define	IS_FSTK_FULL()	(fstkp>=FSTK_SIZE-1)

static	char	emsg[ 256 ];

static	FILE	*pushfile( char [] );
static	FILE	*popfile();
static	FILE	*include();
static	void	dumpfstk( FILE *, char [] );
static	FILE	*getline( char [], int, FILE * );
static	void	putline( FILE *, char [], int, char [] );
static	char	*isdescr( char [] );

char	*RM_preprocessor( void )
{
	FILE	*dfp, *ofp;
	char	line[ 1024 ];
	char	work[ 1024 ];
	char	*dp, *lp;

	rm_wdfname = rm_args->a_dfname;
	if( ( dfp = pushfile( rm_wdfname ) ) == NULL ){
		fprintf( stderr,
			"RM_preprocessor: can't read descr file '%s'.\n",
			rm_args->a_dfname );
		return( NULL );
	}
/*
	if( rm_args->a_xdfname == NULL )
		rm_args->a_xdfname = tempnam( NULL, "rmxd" );
*/
	if( rm_args->a_xdfname == NULL ){
		rm_args->a_xdfname = strdup( "rmxd_XXXXXX" );
		mkstemp( rm_args->a_xdfname );
	}
	if( ( ofp = fopen( rm_args->a_xdfname, "w" ) ) == NULL ){
		fprintf( stderr,
			"RM_preprocessor: can't write temp file '%s'.\n",
			rm_args->a_xdfname );
		return( NULL );
	}

	for( ; dfp = getline( line, sizeof( line ), dfp ); ){
		rm_lineno++;
		if( *line == '#' ){
			for( lp = &line[1]; isspace( *lp ); lp++ )
				;
			if( !strncmp( lp, "include", 7 ) )
				dfp = include( line );		
		}else if( dp = isdescr( line ) ){
			if( rm_args->a_cldefs != NULL ){
				if( dp > line ){
					strncpy( work, line, dp - line );
					work[ dp - line ] = '\0';
					putline( ofp,
						rm_wdfname, rm_lineno, work );
				}
				putline( ofp, "cmd line defs", 1,
					rm_args->a_cldefs );
				putline( ofp, rm_wdfname, rm_lineno, dp );
			}else
				putline( ofp, rm_wdfname, rm_lineno, line );
		}else{
			putline( ofp, rm_wdfname, rm_lineno, line );
		}
	} 
	fclose( ofp );

	return( rm_args->a_xdfname );
}

static	FILE	*pushfile( char fname[] )
{
	char	*sp;
	FILE	*fp;
	FSTK_T	*fsp;

	if( IS_FSTK_FULL() ){
		sprintf( emsg, "pushfile: fstk overflow: '%s'.", fname );
		rm_emsg_lineno = rm_lineno;
		RM_errormsg( TRUE, emsg );
	}
	sp = ( char * )malloc( strlen( fname ) + 1 );
	if( sp == NULL ){
		sprintf( emsg, "RM_pushfile: can't allocate fname for: '%s'.",
			fname );
		rm_emsg_lineno = rm_lineno;
		RM_errormsg( TRUE, emsg );
	}
	strcpy( sp, fname );
	if( ( fp = fopen( fname, "r" ) ) == NULL ){
		sprintf( emsg, "RM_pushfile: can't read '%s'.", fname );
		rm_emsg_lineno = rm_lineno;
		RM_errormsg( TRUE, emsg );
	}
	if( !IS_FSTK_EMPTY() )
		fstk[ fstkp ].f_lineno = rm_lineno;
	fstkp++;
	fsp = &fstk[ fstkp ];
	fsp->f_fname = sp;
	fsp->f_fp = fp;
	fsp->f_lineno = 0;
	rm_wdfname = fsp->f_fname;
	rm_lineno = 0;

	return( fp );
}

static	FILE	*popfile( void )
{
	FSTK_T	*fsp;

	if( IS_FSTK_EMPTY() )
		return( NULL );

	fsp = &fstk[ fstkp ];
	fclose( fsp->f_fp );
	free( fsp->f_fname );
	fstkp--;
	if( IS_FSTK_EMPTY() )
		return( NULL );

	fsp = &fstk[ fstkp ];
	rm_wdfname = fsp->f_fname;
	rm_lineno = fsp->f_lineno;

	return( fsp->f_fp );
}

static	FILE	*include( char str[] )
{
	char	*sp, *sp1, *sp2;
	char	work[ 256 ], fname[ 256 ], path[ 256 ];
	int	c;
	FILE	*fp;
	INCDIR_T	*idp;

	for( sp = &str[1]; isspace( *sp ); sp++ )
		;
	for( ; *sp && !isspace( *sp ); sp++ )
		;
	if( *sp == '\0' ){
		RM_errormsg( TRUE, "RM_include: no filename." );
	}
	for( ; isspace( *sp ); sp++ )
		;
	if( *sp == '"' )
		c = '"';
	else if( *sp == '<' )
		c = '>';
	else if( *sp == '\'' )
		c = '\'';
	else{
		sprintf( emsg, "RM_include: bad include filename '%s'.", sp );
		rm_emsg_lineno = rm_lineno;
		RM_errormsg( TRUE, emsg );
	}
	sp++;
	if( ( sp1 = strchr( sp, c ) ) == NULL ){
		sprintf( emsg, "RM_include: bad include filename '%s'.", sp );
		rm_emsg_lineno = rm_lineno;
		RM_errormsg( TRUE, emsg );
	}
	strncpy( work, sp, sp1 - sp );
	work[ sp1 - sp ] = '\0';
	if( c == '"' ){
		sp2 = RM_str2seq( work );
		strcpy( fname, sp2 );
	}else
		strcpy( fname, work );

/*
	if( rm_idlist == NULL )
*/
	if( rm_args->a_idlist == NULL )
		fp = pushfile( fname );
	else{
		for( idp = rm_args->a_idlist; idp; idp = idp->i_next ){
			sprintf( path, "%s/%s", idp->i_name, fname );
			if( fp = pushfile( path ) )
				break;
		}
	}
	if( fp == NULL ){
		sprintf( emsg,
			"RM_include: can't find include file '%s'.", fname );
		rm_emsg_lineno = rm_lineno;
		RM_errormsg( TRUE, emsg );
	}
	
	return( fp );
}

static	void	dumpfstk( FILE *fp, char msg[] )
{
	int	f;	
	FSTK_T	*fsp;

	fprintf( fp, "%s FSTK: %d entries\n", msg, fstkp + 1 );
	for( f = fstkp; f >= 0; f-- ){
		fsp = &fstk[ f ];
		fprintf( fp, "fstk[%2d] = '%s', line = %d\n",
			f, fsp->f_fname, fsp->f_lineno );
			
	} 
}

static	FILE	*getline( char line[], int l_size, FILE *fp )
{

	do{
		if( fgets( line, l_size, fp ) )
			return( fp );
	}while( fp = popfile() );
	return( NULL );
}
static	void	putline( FILE *fp, char fname[], int lineno, char line[] ) 
{

	fprintf( fp, "\n# line %d '%s'\n", lineno, fname );
	fprintf( fp, "%s", line );
}

static	char	*isdescr( char line[] )
{
	char	*dp, *edp, *qp, *qp1;
	int	instr;

	if( ( dp = strstr( line, "descr" ) ) == NULL ) 
		return( NULL );
	edp = dp + 5;
	if( !isspace( *edp ) && *edp != '#' )
		return( NULL );
	if( dp == line ) 
		return( line );
	if( !isspace( dp[-1] ) && dp[-1] != ';' )
		return( NULL );
	if( ( qp = strchr( line, '\'' ) ) != NULL ){
		if( qp < dp ){
			for( instr = TRUE, qp1 = qp + 1; qp1 < dp; qp1++ ){
				if( *qp1 == '\'' )
					instr ^= TRUE;
				else if( *qp1 == '\\' )
					qp1++;
			}
			return( !instr ? dp : NULL );
		}
	}
	if( ( qp = strchr( line, '"' ) ) != NULL ){
		if( qp < dp ){
			for( instr = TRUE, qp1 = qp + 1; qp1 < dp; qp1++ ){
				if( *qp1 == '"' )
					instr ^= TRUE;
				else if( *qp1 == '\\' )
					qp1++;
			}
			return( instr ? dp : NULL );
		}
	}
	return( dp );
}
