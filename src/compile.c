#include <stdio.h>
#include <string.h>

#include "rnamot.h"

static	STREL_T	*stp = NULL;

void	SE_new( stype )
int	stype;
{

fprintf( stderr, "SE_new: %d.\n", stype );
	stp = ( STREL_T * )malloc( sizeof( STREL_T ) );
	if( stp == NULL ){
		fprintf( stderr, "SE_new: FATAL: can't alloc stp.\n" );
		exit( 1 );
	}
	stp->s_type = stype;
	stp->s_tag = NULL;
	stp->s_next = NULL;
	stp->s_pairs = NULL;
	stp->s_minlen = 0;
	stp->s_maxlen = 0;
	stp->s_seq = NULL;
	stp->s_mismatch = 0;
	stp->s_mispair = 0;
	stp->s_pairdata = NULL;
	stp->s_sites = NULL;
fprintf( stderr, "SE_new: exit.\n" );
}

void	SE_addtag( vp )
VALUE_T	*vp;
{
	char	*sp;
	char	digs[ 20 ];

fprintf( stderr, "SE_addtag: enter.\n" );
	if( vp->v_type == T_INT ){
		sprintf( digs, "%d", vp->v_value.v_ival );
		sp = ( char * )malloc( strlen( digs ) + 1 );
		if( sp == NULL ){
			fprintf( stderr,
				"SE_addtag: T_INT: can't allocate sp.\n" );
			exit( 1 );
		}
		strcpy( sp, digs );
		stp->s_tag = sp;
	}else if( vp->v_type == T_STRING ){
		sp = ( char * )malloc( strlen( vp->v_value.v_cval ) + 1 );
		if( sp == NULL ){
			fprintf( stderr,
				"SE_addtag: T_STRING: can't allocate sp.\n" );
			exit( 1 );
		}
		strcpy( sp, vp->v_value.v_cval );
		stp->s_tag = sp;
	}else{
		fprintf( stderr, "SE_addtag: Unknown value type %d.\n",
			vp->v_type );
		exit( 1 );
	}
fprintf( stderr, "SE_addtag: '%s'\n", stp->s_tag );
}

void	SE_close()
{

fprintf( stderr, "SE_close.\n" );
}
