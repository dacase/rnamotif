#include <stdio.h>
#include <string.h>

#include "rnamot.h"
#include "y.tab.h"

#define	VALSTKSIZE	20
static	VALUE_T	valstk[ VALSTKSIZE ];
static	int	n_valstk;

#define	GLOBAL_IDS_SIZE	50
static	IDENT_T	global_ids[ GLOBAL_IDS_SIZE ] = {
	{ "wc", T_PAIR, C_VAR, S_GLOBAL, { T_PAIR, NULL } },
	{ "gu", T_PAIR, C_VAR, S_GLOBAL, { T_PAIR, NULL } },
	{ "tr", T_PAIR, C_VAR, S_GLOBAL, { T_PAIR, NULL } },
	{ "qu", T_PAIR, C_VAR, S_GLOBAL, { T_PAIR, NULL } },
	{ "overlap", T_INT, C_VAR, S_GLOBAL, { T_INT, 0 } },
	{ "db", T_STRING, C_VAR, S_GLOBAL, { T_STRING, NULL } }
};
static	int	n_global_ids = 6;

#define	LOCAL_IDS_SIZE	20
static	IDENT_T	*local_ids[ LOCAL_IDS_SIZE ];
static	int	n_local_ids;

#define	DESCRSIZE 100
static	STREL_T	descr[ DESCRSIZE ];
static	int	n_descr;
static	STREL_T	*stp;

void	SE_dump();
void	SE_dump_descr();

static	void	enter_id();
static	IDENT_T	*find_id();
static	void	eval();
static	int	loadidval();
static	void	storeexprval();

void	SE_open( stype )
int	stype;
{
	VALUE_T	val;

	n_valstk = 0;
	if( n_descr == DESCRSIZE ){
		fprintf( stderr,
		"SE_new: FATAL: descr array size(%d) exceeded.\n",
			DESCRSIZE );
		exit( 1 );
	}
	stp = &descr[ n_descr ];
	n_descr++;
	stp->s_type = stype;
	stp->s_index = n_descr - 1;
	stp->s_tag = NULL;
	stp->s_next = NULL;
	stp->s_pairs = NULL;
	stp->s_minlen = UNDEF;
	stp->s_maxlen = UNDEF;
	stp->s_seq = NULL;
	stp->s_mismatch = 0;
	stp->s_mispair = 0;
	stp->s_pairdata = NULL;
	stp->s_sites = NULL;
	
	n_local_ids = 0;
	val.v_type = T_STRING;
	val.v_value.v_pval = NULL;
	enter_id( "tag", T_STRING, C_VAR, S_LOCAL, &val );

	val.v_type = T_INT;
	val.v_value.v_ival = UNDEF;
	enter_id( "minlen", T_INT, C_VAR, S_LOCAL, &val );

	val.v_type = T_INT;
	val.v_value.v_ival = UNDEF;
	enter_id( "maxlen", T_INT, C_VAR, S_LOCAL, &val );

	val.v_type = T_STRING;
	val.v_value.v_pval = NULL;
	enter_id( "seq", T_STRING, C_VAR, S_LOCAL, &val );

	val.v_type = T_INT;
	val.v_value.v_ival = 0;
	enter_id( "mismatch", T_INT, C_VAR, S_LOCAL, &val );

	val.v_type = T_INT;
	val.v_value.v_ival = 0;
	enter_id( "mispair", T_INT, C_VAR, S_LOCAL, &val );

	val.v_type = T_PAIR;
	val.v_value.v_pval = NULL;
	enter_id( "pair", T_PAIR, C_VAR, S_LOCAL, &val );
}

void	SE_addval( expr )
NODE_T	*expr;
{
	int	op;
	NODE_T	*npl, *npr;
	char	*id;
	char	*sp;

	n_valstk = 0;
	eval( expr, 0 );
}

void	SE_close()
{
	int	i;
	IDENT_T	*ip;

	for( i = 0; i < n_local_ids; i++ ){
		ip = local_ids[ i ];
		if( !strcmp( ip->i_name, "tag" ) ){
			stp->s_tag = ip->i_val.v_value.v_pval;
		}else if( !strcmp( ip->i_name, "minlen" ) ){
			stp->s_minlen = ip->i_val.v_value.v_ival;
		}else if( !strcmp( ip->i_name, "maxlen" ) ){
			stp->s_maxlen = ip->i_val.v_value.v_ival;
		}else if( !strcmp( ip->i_name, "seq" ) ){
			stp->s_seq = ip->i_val.v_value.v_pval;
		}else if( !strcmp( ip->i_name, "mismatch" ) ){
			stp->s_mismatch = ip->i_val.v_value.v_ival;
		}else if( !strcmp( ip->i_name, "mispair" ) ){
			stp->s_mispair = ip->i_val.v_value.v_ival;
		}else if( !strcmp( ip->i_name, "pair" ) ){
			stp->s_pairdata = NULL;
		}
	}
}

void	SE_dump( fp, d_pair, d_parm, d_descr, d_site )
FILE	*fp;
int	d_pair;
int	d_parm;
int	d_descr;
int	d_site;
{
	STREL_T	*stp;
	int	i;

	if( d_descr ){
		fprintf( stderr, "DESCR: %3d structure elements.\n", n_descr );
		for( stp = descr, i = 0; i < n_descr; i++, stp++ )
			SE_dump_descr( fp, stp );
	}
}
void	SE_dump_descr( fp, stp )
FILE	*fp;
STREL_T	*stp;
{

	fprintf( fp, "descr[%3d] = {\n", stp->s_index + 1 );
	fprintf( fp, "\ttype = " );
	switch( stp->s_type ){
	case SYM_SS :
		fprintf( fp, "ss" );
		break;
	case SYM_H5 :
		fprintf( fp, "h5" );
		break;
	case SYM_H3 :
		fprintf( fp, "h3" );
		break;
	case SYM_P5 :
		fprintf( fp, "p5" );
		break;
	case SYM_P3 :
		fprintf( fp, "p3" );
		break;
	case SYM_T1 :
		fprintf( fp, "t1" );
		break;
	case SYM_T2 :
		fprintf( fp, "t2" );
		break;
	case SYM_T3 :
		fprintf( fp, "t3" );
		break;
	case SYM_Q1 :
		fprintf( fp, "q1" );
		break;
	case SYM_Q2 :
		fprintf( fp, "q2" );
		break;
	case SYM_Q3 :
		fprintf( fp, "q3" );
		break;
	case SYM_Q4 :
		fprintf( fp, "q4" );
		break;
	default :
		fprintf( fp, "unknown (%d)", stp->s_type );
		break;
	}
	fprintf( fp, "\n" );

	fprintf( fp, "\ttag  = '%s'\n",
		stp->s_tag ? stp->s_tag : "(No tag)" );

	fprintf( fp, "\tlen  = " );
	if( stp->s_minlen == LASTVAL )
		fprintf( fp, "LASTVAL" );
	else
		fprintf( fp, "%d", stp->s_minlen );
	fprintf( fp, ":" );
	if( stp->s_maxlen == LASTVAL )
		fprintf( fp, "LASTVAL" );
	else
		fprintf( fp, "%d", stp->s_maxlen );
	fprintf( fp, "\n" );

	fprintf( fp, "\tseq  = '%s'\n",
		stp->s_seq ? stp->s_seq : "(No seq)" );

	fprintf( fp, "\tmismatch = %d\n", stp->s_mismatch );

	fprintf( fp, "\tmispair = %d\n", stp->s_mispair );

	fprintf( fp, "}\n" );
}

static	void	enter_id( name, type, class, scope, vp )
char	name[];
int	type;
int	class;
VALUE_T	*vp;
{
	IDENT_T	*ip;
	char	*np;

	if( scope == S_GLOBAL ){
		if( n_global_ids >= GLOBAL_IDS_SIZE ){
			fprintf( stderr,
			"enter_id: FATAL: global symbol tab overflow.\n" );
			exit( 1 );
		}
		ip = &global_ids[ n_global_ids ];
		n_global_ids++;
	}else{
		if( n_local_ids >= LOCAL_IDS_SIZE ){
			fprintf( stderr,
			"enter_id: FATAL: local symbol tab overflow.\n" );
			exit( 1 );
		}
		ip = ( IDENT_T * )malloc( sizeof( IDENT_T ) );
		if( ip == NULL ){
			fprintf( stderr,
				"enter_id: FATAL: can't alloc local ip.\n" );
			exit( 1 );
		}
		local_ids[ n_local_ids ] = ip;
		n_local_ids++;
	}
	np = ( char * )malloc( strlen( name ) + 1 );
	if( np == NULL ){
		fprintf( stderr,
			"enter_id: FATAL: can't alloc np for name.\n" );
		exit( 1 );
	}
	strcpy( np, name );
	ip->i_name = np;
	ip->i_type = type;
	ip->i_class = class;
	ip->i_scope = scope;
	ip->i_val.v_type = type;
	if( type == T_INT ){
		ip->i_val.v_value.v_ival = vp->v_value.v_ival;
	}else if( type == T_STRING ){
		if( vp->v_value.v_pval == NULL ) 
			ip->i_val.v_value.v_pval = NULL;
		else{
			np = ( char * )
				malloc(strlen(vp->v_value.v_pval)+1);
			if( np == NULL ){
				fprintf( stderr,
			"enter_id: FATAL: can't alloc np for string val.\n" );
				exit( 1 );
			}
			strcpy( np, vp->v_value.v_pval );
			ip->i_val.v_value.v_pval = np;
		}
	}else if( type == T_PAIR ){
		ip->i_val.v_value.v_pval = NULL;
	}
}

static	IDENT_T	*find_id( name )
char	name[];
{
	int	i;
	IDENT_T	*ip;
	
	for( i = 0; i < n_local_ids; i++ ){
		ip = local_ids[ i ];
		if( !strcmp( name, ip->i_name ) )
			return( ip );
	}
	for( ip = global_ids, i = 0; i < n_global_ids; i++, ip++ ){
		if( !strcmp( name, ip->i_name ) )
			return( ip );
	}
	return( NULL );
}

static	void	eval( expr )
NODE_T	*expr;
{
	char	*sp, *l_sp, *r_sp;
	IDENT_T	*ip, *ip1;
	int	l_type, r_type;

	if( expr ){
		eval( expr->n_left );
		eval( expr->n_right );
		switch( expr->n_sym ){
		case SYM_INT :
			valstk[ n_valstk ].v_type = T_INT;
			valstk[ n_valstk ].v_value.v_ival =
				expr->n_val.v_value.v_ival;
			n_valstk++;
			break;
		case SYM_STRING :
			sp = ( char * )
				malloc(strlen( expr->n_val.v_value.v_pval )+1);
			if( sp == NULL ){
				fprintf( stderr,
			"eval: FATAL: can't allocate sp for string.\n" );
				exit( 1 );
			}
			strcpy( sp, expr->n_val.v_value.v_pval );
			valstk[ n_valstk ].v_type = T_STRING;
			valstk[ n_valstk ].v_value.v_pval = sp;
			n_valstk++;
			break;
		case SYM_IDENT :
			ip = find_id( expr->n_val.v_value.v_pval );
			if( ip == NULL ){
				fprintf( stderr,
					"eval: FATAL: unknown id '%s'.\n",
					expr->n_val.v_value.v_pval );
				exit( 1 );
			}
			valstk[ n_valstk ].v_type = T_IDENT;
			valstk[ n_valstk ].v_value.v_pval = ip;
			n_valstk++;
			break;
		case SYM_PLUS :
			l_type = valstk[ n_valstk - 2 ].v_type;
			if( l_type == T_IDENT )
				l_type = loadidval( &valstk[ n_valstk - 2 ] );
			r_type = valstk[ n_valstk - 1 ].v_type;
			if( r_type == T_IDENT )
				r_type = loadidval( &valstk[ n_valstk - 1 ] );
			if( l_type != r_type ){
				fprintf( stderr,
					"eval: FATAL: type mismatch '+'\n" );
				exit( 1 );
			}
			if( l_type == T_INT ){
				valstk[ n_valstk - 2 ].v_value.v_ival +=
					valstk[ n_valstk - 1 ].v_value.v_ival;
			}else if( l_type == T_STRING ){
				l_sp = valstk[ n_valstk - 2 ].v_value.v_pval;
				r_sp = valstk[ n_valstk - 1 ].v_value.v_pval;
				sp = ( char * )malloc( strlen( l_sp ) +
					strlen( r_sp ) + 1 );
				if( sp == NULL ){
					fprintf( stderr,
				"eval: FATAL: can't alloc sp for str +.\n" );
					exit( 1 );
				}
				strcpy( sp, l_sp );
				strcat( sp, r_sp );
				valstk[ n_valstk - 2 ].v_value.v_pval = sp;
			}else if( l_type == T_PAIR ){
			}
			n_valstk--;
			break;
		case SYM_MINUS :
			l_type = valstk[ n_valstk - 2 ].v_type;
			if( l_type == T_IDENT )
				l_type = loadidval( &valstk[ n_valstk - 2 ] );
			r_type = valstk[ n_valstk - 1 ].v_type;
			if( r_type == T_IDENT )
				r_type = loadidval( &valstk[ n_valstk - 1 ] );
			if( l_type != r_type ){
				fprintf( stderr,
					"eval: FATAL: type mismatch '-'\n" );
				exit( 1 );
			}
			if( l_type == T_INT ){
				valstk[ n_valstk - 2 ].v_value.v_ival -=
					valstk[ n_valstk - 1 ].v_value.v_ival;
			}else if( l_type == T_STRING ){
				fprintf( stderr,
			"eval: FATAL: op '-' not defined for strings.\n" );
				exit( 1 );
			}else if( l_type == T_PAIR ){
			}
			n_valstk--;
			break;
		case SYM_ASSIGN :
			ip = valstk[ n_valstk - 2 ].v_value.v_pval;
			l_type = ip->i_type;
			r_type = valstk[ n_valstk - 1 ].v_type;
			if( r_type == T_IDENT )
				r_type = loadidval( &valstk[ n_valstk - 1 ] );
			if( l_type != r_type ){
				fprintf( stderr,
					"eval: FATAL: type mismatch '='\n" );
				exit( 1 );
			}
			storeexprval( ip, &valstk[ n_valstk-1 ] );
			n_valstk -= 2;
			break;
		case SYM_PLUS_ASSIGN :
			ip = valstk[ n_valstk - 2 ].v_value.v_pval;
			l_type = loadidval( &valstk[ n_valstk - 2 ] );
			r_type = valstk[ n_valstk - 1 ].v_type;
			if( r_type == T_IDENT )
				r_type = loadidval( &valstk[ n_valstk - 1 ] );
			if( l_type != r_type ){
				fprintf( stderr,
					"eval: FATAL: type mismatch '+='\n" );
				exit( 1 );
			}
			if( l_type == T_INT ){
				valstk[ n_valstk - 2 ].v_value.v_ival +=
					valstk[ n_valstk - 1 ].v_value.v_ival;
			}else if( l_type == T_STRING ){
				l_sp = valstk[ n_valstk - 2 ].v_value.v_pval;
				r_sp = valstk[ n_valstk - 1 ].v_value.v_pval;
				sp = ( char * )malloc( strlen( l_sp ) +
					strlen( r_sp ) + 1 );
				if( sp == NULL ){
					fprintf( stderr,
				"eval: FATAL: can't alloc sp for str +.\n" );
					exit( 1 );
				}
				strcpy( sp, l_sp );
				strcat( sp, r_sp );
				valstk[ n_valstk - 2 ].v_value.v_pval = sp;
			}else if( l_type == T_PAIR ){
			}
			storeexprval( ip, &valstk[ n_valstk - 2 ] );
			n_valstk -= 2;
			break;
		case SYM_MINUS_ASSIGN :
			ip = valstk[ n_valstk - 2 ].v_value.v_pval;
			l_type = loadidval( &valstk[ n_valstk - 2 ] );
			r_type = valstk[ n_valstk - 1 ].v_type;
			if( r_type == T_IDENT )
				r_type = loadidval( &valstk[ n_valstk - 1 ] );
			if( l_type != r_type ){
				fprintf( stderr,
					"eval: FATAL: type mismatch '-='\n" );
				exit( 1 );
			}
			if( l_type == T_INT ){
				valstk[ n_valstk - 2 ].v_value.v_ival -=
					valstk[ n_valstk - 1 ].v_value.v_ival;
			}else if( l_type == T_STRING ){
				fprintf( stderr,
			"eval: FATAL: op '-' not defined for strings.\n" );
				exit( 1 );
			}else if( l_type == T_PAIR ){
			}
			storeexprval( ip, &valstk[ n_valstk - 2 ] );
			n_valstk -= 2;
			break;
		}
	}
}

static	int	loadidval( vp )
VALUE_T	*vp;
{
	int	type;
	IDENT_T	*ip;
	char	*sp;

	ip = vp->v_value.v_pval;
	type = ip->i_type;
	if( type == T_INT ){
		vp->v_type = T_INT;
		vp->v_value.v_ival = ip->i_val.v_value.v_ival;
	}else if( type == T_STRING ){
		vp->v_type = T_STRING;
		sp = ( char * )malloc( strlen( ip->i_val.v_value.v_pval ) + 1 );
		if( sp == NULL ){
			fprintf( stderr,
				"loadidval: FATAL: can't allocate sp.\n" );
			exit( 1 );
		}
		strcpy( sp, ip->i_val.v_value.v_pval );
		vp->v_value.v_pval = sp;
	}else if( type == T_PAIR ){
		vp->v_type = T_PAIR;
	}
	return( type );
}

static	void	storeexprval( ip, vp )
IDENT_T	*ip;
VALUE_T	*vp;
{
	int	type;
	char	*sp;

	type = vp->v_type;
	if( type == T_INT ){
		ip->i_type = T_INT;
		ip->i_val.v_value.v_ival = vp->v_value.v_ival;
	}else if( type == T_STRING ){
		ip->i_type = T_STRING;
		sp = ( char * )malloc( strlen( vp->v_value.v_pval ) + 1 );
		if( sp == NULL ){
			fprintf( stderr,
				"storeexprval: FATAL: can't allocate sp.\n" );
			exit( 1 );
		}
		strcpy( sp, vp->v_value.v_pval ); 
		ip->i_val.v_value.v_pval = sp;
	}else if( type == T_PAIR ){
		ip->i_type = T_PAIR;
	}
}
