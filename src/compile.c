#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "rnamot.h"
#include "y.tab.h"

extern	char	rm_fname[ 256 ];
extern	int	rm_lineno;
extern	int	rm_error;
extern	int	rm_emsg_lineno;
extern	VALUE_T	rm_tokval;

static	char	emsg[ 256 ];

#define	VALSTKSIZE	20
static	VALUE_T	valstk[ VALSTKSIZE ];
static	int	n_valstk;

extern	IDENT_T	rm_global_ids[];
extern	int	rm_s_global_ids;
extern	int	rm_n_global_ids;

#define	LOCAL_IDS_SIZE	20
static	IDENT_T	*local_ids[ LOCAL_IDS_SIZE ];
static	int	n_local_ids;

#define	ISBASE(b)	((b)=='a'||(b)=='c'||(b)=='g'||(b)=='t'||(b)=='u'|| \
			 (b)=='A'||(b)=='C'||(b)=='G'||(b)=='T'||(b)=='U')
#define	CURPAIR_SIZE	20
static	char	*curpair[ CURPAIR_SIZE ];
int	n_curpair;

PAIRLIST_T	*pairlists[ 5 ];	/* #str = (0,1,) 2,3,4	*/

extern	STREL_T	rm_descr[];
extern	int	rm_s_descr;
extern	int	rm_n_descr;
static	STREL_T	*stp;
static	PAIRSET_T	*def_pairset = NULL;

extern	POS_T	rm_pos[];
extern	int	rm_s_pos;
extern	int	rm_n_pos;
static	POS_T	*posp;

NODE_T	*PR_close();

static	IDENT_T	*enter_id();
static	IDENT_T	*find_id();
static	void	eval();
static	int	loadidval();
static	void	storeexprval();
static	PAIRSET_T	*pairop();
static	POS_T	*posop();

static	void	seqlen();
static	int	link_tags();
static	void	chk_tagorder();
static	void	mk_links();
static	void	duptags_error();
static	int	chk_parms();

int	RM_init()
{
	IDENT_T	*ip;
	NODE_T	*np;

	rm_lineno = 0;
	curpair[0] = "a:u";
	curpair[1] = "c:g";
	curpair[2] = "g:c";
	curpair[3] = "u:a";
	n_curpair = 4;
	ip = find_id( "wc" );
	np = PR_close();
	ip->i_val.v_value.v_pval = np->n_val.v_value.v_pval;

	curpair[0] = "g:u";
	curpair[1] = "u:g";
	n_curpair = 2;
	ip = find_id( "gu" );
	np = PR_close();
	ip->i_val.v_value.v_pval = np->n_val.v_value.v_pval;

	curpair[0] = "a:u:u";
	n_curpair = 1;
	ip = find_id( "tr" );
	np = PR_close();
	ip->i_val.v_value.v_pval = np->n_val.v_value.v_pval;

	curpair[0] = "g:g:g:g";
	n_curpair = 1;
	ip = find_id( "qu" );
	np = PR_close();
	ip->i_val.v_value.v_pval = np->n_val.v_value.v_pval;

	rm_lineno = 1;
}

PARM_add( expr )
NODE_T	*expr;
{

	n_valstk = 0;
	eval( expr, 1 );
}

void	PR_open()
{

	n_curpair = 0;
}

void	PR_add( np )
NODE_T	*np;
{

	if( n_curpair >= CURPAIR_SIZE )
		errormsg( 1, "PR_add: current pair too large." );
	curpair[ n_curpair ] = np->n_val.v_value.v_pval;
	n_curpair++;
}

NODE_T	*PR_close()
{
	int	i, b, needbase;
	PAIR_T	*pp;
	PAIRSET_T	*ps;
	char	*bp;
	NODE_T	*np;

	ps = ( PAIRSET_T * )malloc( sizeof( PAIRSET_T ) );
	if( ps == NULL )
		errormsg( 1, "PR_close: can't allocate pairlist." );
	pp = ( PAIR_T * )malloc( n_curpair * sizeof( PAIR_T ) );
	if( pp == NULL )
		errormsg( 1, "PR_close: can't allocate pair." );
	ps->ps_n_pairs = n_curpair;
	ps->ps_pairs = pp;
	for( pp = ps->ps_pairs, i = 0; i < ps->ps_n_pairs; i++, pp++ ){
		for( needbase = 1, b = 0, bp = curpair[ i ]; *bp; bp++ ){
			if( ISBASE( *bp ) ){
				if( needbase ){
					if( b >= 4 ){
						errormsg( 0,
			"PR_close: At most 4 bases in a pair-string." );
						break;
					}else{
						pp->p_n_bases = b + 1;
						pp->p_bases[ b ]= *bp;
						b++;
						needbase = 0;
					}
				}else{
					errormsg( 0,
		"PR_close: pair-string is base-letter : base-letter : ..." );
					break;
				}
			}else if( *bp == ':' ){
				if( needbase ){
					errormsg( 0,
		"PR_close: pair-string is base-letter : base-letter : ..." );
					break;
				}
				needbase = 1;
			}else{
				errormsg( 0,
		"PR_close: pair-string is base-letter : base-letter : ..." );
				break;
			}
		}
		if( pp->p_n_bases < 2 || pp->p_n_bases > 4 ){
			errormsg( 0, "PR_close: pair-string has 2-4 bases." );
		}
	}
	ps = pairop( "check", ps, NULL );

	np = ( NODE_T * )malloc( sizeof( NODE_T ) );
	if( np == NULL ){
		errormsg( 1, "PR_close: can't allocate np." );
	}
	np->n_sym = SYM_LCURLY;
	np->n_type = T_PAIR;
	np->n_class = C_LIT;
	np->n_lineno = rm_lineno;
	np->n_val.v_type = T_PAIR;
	np->n_val.v_value.v_pval = ps;
	np->n_left = NULL;
	np->n_right = NULL;
	return( np );
}

void	SE_open( stype )
int	stype;
{
	VALUE_T	val;
	IDENT_T	*ip;
	PAIRSET_T *ps;

	n_valstk = 0;
	if( rm_n_descr == rm_s_descr ){
		rm_emsg_lineno = rm_lineno;
		sprintf( emsg, "SE_new: descr array size(%d) exceeded.",
			rm_s_descr );
		errormsg( 1, emsg );
	}
	stp = &rm_descr[ rm_n_descr ];
	rm_n_descr++;
	stp->s_checked = 0;
	stp->s_type = stype;
	stp->s_index = rm_n_descr - 1;
	stp->s_lineno = rm_lineno;
	stp->s_tag = NULL;
	stp->s_next = NULL;
	stp->s_mates = NULL;
	stp->s_n_mates = 0;
	stp->s_minlen = UNDEF;
	stp->s_maxlen = UNDEF;
	stp->s_seq = NULL;
	stp->s_mismatch = 0;
	stp->s_mispair = 0;
	stp->s_pairset = NULL;
	stp->s_sites = NULL;
	stp->s_n_sites = 0;
	
	n_local_ids = 0;
	val.v_type = T_STRING;
	val.v_value.v_pval = NULL;
	ip = enter_id( "tag", T_STRING, C_VAR, S_STREL, &val );

	val.v_type = T_INT;
	val.v_value.v_ival = UNDEF;
	ip = enter_id( "minlen", T_INT, C_VAR, S_STREL, &val );

	val.v_type = T_INT;
	val.v_value.v_ival = UNDEF;
	ip = enter_id( "maxlen", T_INT, C_VAR, S_STREL, &val );

	val.v_type = T_STRING;
	val.v_value.v_pval = NULL;
	ip = enter_id( "seq", T_STRING, C_VAR, S_STREL, &val );

	val.v_type = T_INT;
	val.v_value.v_ival = 0;
	ip = enter_id( "mismatch", T_INT, C_VAR, S_STREL, &val );

	if( stype != SYM_SS ){ 
		val.v_type = T_INT;
		val.v_value.v_ival = 0;
		ip = enter_id( "mispair", T_INT, C_VAR, S_STREL, &val );

		switch( stype ){
		case SYM_SS :
			stp->s_pairset = NULL;
			break;
		case SYM_H5 :
		case SYM_H3 :
		case SYM_P5 :
		case SYM_P3 :
			ip = find_id( "wc" );
			def_pairset = ip->i_val.v_value.v_pval;
			break;
		case SYM_T1 :
		case SYM_T2 :
		case SYM_T3 :
			ip = find_id( "tr" );
			def_pairset = ip->i_val.v_value.v_pval;
			break;
		case SYM_Q1 :
		case SYM_Q2 :
		case SYM_Q3 :
		case SYM_Q4 :
			ip = find_id( "qu" );
			def_pairset = ip->i_val.v_value.v_pval;
			break;
		}
		val.v_type = T_PAIR;
		val.v_value.v_pval = NULL;
		ip = enter_id( "pair", T_PAIR, C_VAR, S_STREL, &val );
	}
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
	int	i, minlen, maxlen;
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
			stp->s_pairset = ip->i_val.v_value.v_pval;
		}
	}
	def_pairset = NULL;
}

int	SE_link( n_descr, descr )
int	n_descr;
STREL_T	descr[];
{

	if( n_descr == 0 ){
		errormsg( 0, "SE_link: Descriptor has 0 elements." );
		return( 1 );
	}

	if( link_tags( n_descr, descr ) )
		return( 1 );

	if( chk_parms( n_descr, descr ) )
		return( 1 );

	return( 0 );
}

static	int	link_tags( n_descr, descr )
int	n_descr;
STREL_T	descr[];
{
	int	i, j;
	STREL_T	*stp, *stp1;
	STREL_T	*tags[ 4 ];
	int	n_tags;
	STREL_T	*tstk[ 50 ];
	int	n_tstk;
	char	*tp;

	/* insure that all triple & quadriple helix els. are tagged	*/
	for( stp = descr, i = 0; i < n_descr; i++, stp++ ){
		if( stp->s_type == SYM_SS ||
			stp->s_type == SYM_H5 ||
			stp->s_type == SYM_H3 ||
			stp->s_type == SYM_P5 ||
			stp->s_type == SYM_P3 )
		{
			continue;
		}
		if( stp->s_tag == NULL ){
			rm_emsg_lineno = stp->s_lineno;
			errormsg( 0,
			"all triple/quad. helix els. must be tagged." );
		}
	}

	/* link all explicitly tagged elements	*/
	for( stp = descr, i = 0; i < n_descr; i++, stp++ ){
		if( stp->s_checked )
			continue;
		stp->s_checked = 1;
		if( stp->s_tag == NULL )
			continue;
		tp = stp->s_tag;
		tags[ 0 ] = stp;
		n_tags = 1;
		for( j = i + 1; j < n_descr; j++ ){
			stp1 = &descr[ j ];
			if( stp1->s_checked )
				continue;
			if( stp1->s_tag == NULL )
				continue;
			if( strcmp( tp, stp1->s_tag ) )
				continue;
			stp1->s_checked = 1;
			if( n_tags < 4 )
				tags[ n_tags ] = stp1;
			n_tags++;
		}
		chk_tagorder( n_tags, tags );
	}

	/* link remaining untagged duplexes	*/
	for( n_tstk = 0, stp = descr, i = 0; i < n_descr; i++, stp++ ){
		if( stp->s_tag != NULL )
			continue;
		if( stp->s_type == SYM_SS )
			continue;
		else if( stp->s_type == SYM_H5 || stp->s_type == SYM_P5 ){
			tstk[ n_tstk ] = stp;
			n_tstk++;
		}else if( stp->s_type == SYM_H3 || stp->s_type == SYM_P3 ){
			if( n_tstk == 0 ){
				rm_emsg_lineno = stp->s_lineno;
				sprintf( emsg,
				"%s element has no matching %s element.",
					stp->s_type == SYM_H3 ? "h3" : "h5",
					stp->s_type == SYM_H3 ? "p3" : "p5" );
				errormsg( 0, emsg );
			}else{
				tags[ 0 ] = tstk[ n_tstk - 1 ];
				n_tstk--;
				tags[ 1 ] = stp;
				n_tags = 2;
				chk_tagorder( n_tags, tags );
			}
		}
	}
	if( n_tstk > 0 ){
		for( i = 0; i < n_tstk; i++ ){
			stp = tstk[ i ];
			rm_emsg_lineno = stp->s_lineno;
			sprintf( emsg,
				"%s element has no matching %s element.",
				stp->s_type == SYM_H5 ? "h5" : "h3",
				stp->s_type == SYM_H5 ? "p5" : "p3" );
		}
	}
	return( rm_error );
}

static	void	chk_tagorder( n_tags, tags )
int	n_tags;
STREL_T	*tags[];
{
	int	t1, t2, t3, t4;

	t1 = tags[ 0 ]->s_type;
	if( t1 == SYM_SS ){
		if( n_tags > 1 )
			duptags_error( 1, n_tags, tags );
	}else if( t1 == SYM_H5 ){
		if( n_tags < 2 ){
			sprintf( emsg, "wc-helix '%s' has no h3() element.", 
				tags[ 0 ]->s_tag );
			rm_emsg_lineno = tags[ 0 ]->s_lineno;
			errormsg( 0, emsg );
		}else{
			t2 = tags[ 1 ]->s_type;
			if( t2 == SYM_H3 ){
				if( n_tags == 2 ){
					mk_links( n_tags, tags );
					return;		/* h5 ... h3 pair, OK */
				}else
					duptags_error( 2, n_tags, tags );
			}else
				duptags_error( 1, n_tags, tags );
		}
	}else if( t1 == SYM_P5 ){
		if( n_tags < 2 ){
			sprintf( emsg,
				"parallel-helix '%s' has no h3() element.", 
				tags[ 0 ]->s_tag );
			rm_emsg_lineno = tags[ 0 ]->s_lineno;
			errormsg( 0, emsg );
		}else{
			t2 = tags[ 1 ]->s_type;
			if( t2 == SYM_P3 ){
				if( n_tags == 2 ){
					mk_links( n_tags, tags );
					return;		/* p5 p3 pair, OK */
				}else
					duptags_error( 2, n_tags, tags );
			}else
				duptags_error( 1, n_tags, tags );
		}
	}else if( t1 == SYM_T1 ){
		if( n_tags < 3 ){
			sprintf( emsg,
				"triplex '%s' is has < 3 elements.",
				tags[ 0 ]->s_tag );
			rm_emsg_lineno = tags[ 0 ]->s_lineno;
			errormsg( 0, emsg );
		}else{
			t2 = tags[ 1 ]->s_type;
			t3 = tags[ 2 ]->s_type;
			if( t2 == SYM_T2 && t3 == SYM_T3 ){
				if( n_tags == 3 ){
					mk_links( n_tags, tags );
					return;
				}else
					duptags_error( 3, n_tags, tags );
			}else
				duptags_error( 2, n_tags, tags );
		}
	}else if( t1 == SYM_Q1 ){
		if( n_tags < 4 ){
			sprintf( emsg,
				"4-plex '%s' is has < 4 elements.",
				tags[ 0 ]->s_tag );
			rm_emsg_lineno = tags[ 0 ]->s_lineno;
			errormsg( 0, emsg );
		}else{
			t2 = tags[ 1 ]->s_type;
			t3 = tags[ 2 ]->s_type;
			t4 = tags[ 3 ]->s_type;
			if( t2 == SYM_Q2 && t3 == SYM_Q3 && t4 == SYM_Q4){
				if( n_tags == 4 ){
					mk_links( n_tags, tags );
					return;
				}else
					duptags_error( 4, n_tags, tags );
			}else
				duptags_error( 3, n_tags, tags );
		}
	}else{
		sprintf( emsg, "1st use of tag '%s' is out of order.",
			tags[ 0 ]->s_tag );
		rm_emsg_lineno = tags[ 0 ]->s_lineno;
		errormsg( 0, emsg );
	}
}

static	void	mk_links( n_tags, tags )
int	n_tags;
STREL_T	*tags[];
{
	int	i, j, k;
	STREL_T	**stp;

	for( i = 0; i < n_tags; i++ ){
		stp = ( STREL_T ** )malloc(( n_tags - 1 )* sizeof( STREL_T * ));
		for( k = 0, j = 0; j < n_tags; j++ ){
			if( j != i ){
				stp[ k ] = tags[ j ];
				k++;
			}
		}
		tags[ i ]->s_mates = stp;
		tags[ i ]->s_n_mates = n_tags - 1;
	}
}

static	void	duptags_error( need, n_tags, tags )
int	need;
int	n_tags;
STREL_T	*tags[];
{
	int	i;

	for( i = need; i < n_tags; i++ ){
		sprintf( emsg, "duplicate tag '%s'.", tags[ i ]->s_tag );
		rm_emsg_lineno = tags[ i ]->s_lineno;
		errormsg( 0, emsg );
	}
}

static	int	chk_parms( n_descr, descr )
int	n_descr;
STREL_T	descr[];
{
	int	i;
	STREL_T	*stp;

	for( stp = descr, i = 0; i < n_descr; i++, stp++ ){
		switch( stp->s_type ){
		case SYM_SS :
			break;
		case SYM_H5 :
			break;
		case SYM_P5 :
			break;
		case SYM_T1 :
			break;
		case SYM_Q1 :
			break;
		}
	}
	return( 0 );
}

static	IDENT_T	*enter_id( name, type, class, scope, vp )
char	name[];
int	type;
int	class;
VALUE_T	*vp;
{
	IDENT_T	*ip;
	char	*np;

	if( scope == S_GLOBAL ){
		if( rm_n_global_ids >= rm_s_global_ids ){
			errormsg( 1, 
				"enter_id: global symbol tab overflow." );
		}
		ip = &rm_global_ids[ rm_n_global_ids ];
		rm_n_global_ids++;
	}else{
		if( n_local_ids >= LOCAL_IDS_SIZE ){
			errormsg( 1, "enter_id: local symbol tab overflow." );
		}
		ip = ( IDENT_T * )malloc( sizeof( IDENT_T ) );
		if( ip == NULL ){
			errormsg( 1, "enter_id: can't alloc local ip." );
		}
		local_ids[ n_local_ids ] = ip;
		n_local_ids++;
	}
	np = ( char * )malloc( strlen( name ) + 1 );
	if( np == NULL ){
		errormsg( 1, "enter_id: can't alloc np for name." );
	}
	strcpy( np, name );
	ip->i_name = np;
	ip->i_type = type;
	ip->i_class = class;
	ip->i_scope = scope;
	ip->i_val.v_type = type;
	ip->i_val.v_value.v_pval = NULL;
	if( type == T_INT ){
		ip->i_val.v_value.v_ival = vp->v_value.v_ival;
	}else if( type == T_STRING ){
		if( vp->v_value.v_pval == NULL ) 
			ip->i_val.v_value.v_pval = NULL;
		else{
			np = ( char * )
				malloc(strlen(vp->v_value.v_pval)+1);
			if( np == NULL ){
				errormsg( 1,
				"enter_id: can't alloc np for string val." );
			}
			strcpy( np, vp->v_value.v_pval );
			ip->i_val.v_value.v_pval = np;
		}
	}else if( type == T_PAIR ){
		ip->i_val.v_value.v_pval = pairop( "copy", 
			vp->v_value.v_pval, NULL );
	}
	return( ip );
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
	for( ip = rm_global_ids, i = 0; i < rm_n_global_ids; i++, ip++ ){
		if( !strcmp( name, ip->i_name ) )
			return( ip );
	}
	return( NULL );
}

static	void	eval( expr, d_ok )
NODE_T	*expr;
int	d_ok;
{
	char	*sp, *l_sp, *r_sp;
	IDENT_T	*ip, *ip1;
	int	l_type, r_type;
	VALUE_T	*vp;
	PAIRSET_T	*n_ps, *l_ps, *r_ps;
	POS_T	*l_pos, *r_pos, *n_pos;

	if( expr ){
		eval( expr->n_left, d_ok );
		eval( expr->n_right, d_ok );
		rm_emsg_lineno = expr->n_lineno;
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
				errormsg( 1,
				"eval: can't allocate sp for string." );
			}
			strcpy( sp, expr->n_val.v_value.v_pval );
			valstk[ n_valstk ].v_type = T_STRING;
			valstk[ n_valstk ].v_value.v_pval = sp;
			n_valstk++;
			break;
		case SYM_LCURLY :
			valstk[ n_valstk ].v_type = T_PAIR;
			valstk[ n_valstk ].v_value.v_pval = 
				expr->n_val.v_value.v_pval;
			n_valstk++;
			break;
		case SYM_DOLLAR :
			valstk[ n_valstk ].v_type = T_POS;
			valstk[ n_valstk ].v_value.v_pval = 
				expr->n_val.v_value.v_pval;
			n_valstk++;
			break;
		case SYM_IDENT :
			ip = find_id( expr->n_val.v_value.v_pval );
			if( ip == NULL ){
				if( d_ok ){
					ip=enter_id(expr->n_val.v_value.v_pval,
						T_UNDEF, C_VAR, S_GLOBAL, NULL);
				}else{
					sprintf( emsg, "eval: unknown id '%s'.",
						expr->n_val.v_value.v_pval );
					errormsg( 1, emsg );
				}
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
				errormsg( 1, "eval: type mismatch '+'." );
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
					errormsg( 1,
					"eval: can't alloc sp for str +." );
				}
				strcpy( sp, l_sp );
				strcat( sp, r_sp );
				valstk[ n_valstk - 2 ].v_value.v_pval = sp;
			}else if( l_type == T_PAIR ){
				l_ps = valstk[ n_valstk - 2 ].v_value.v_pval;
				r_ps = valstk[ n_valstk - 1 ].v_value.v_pval;
				n_ps = pairop( "add", l_ps, r_ps );
				valstk[ n_valstk - 2 ].v_value.v_pval = n_ps;
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
				if( l_type == T_POS && r_type == T_INT ){
					posop("cvt", &valstk[n_valstk-1], NULL);
				}else{
					errormsg( 1,
						"eval: type mismatch '-'." );
				}
			}
			if( l_type == T_INT ){
				valstk[ n_valstk - 2 ].v_value.v_ival -=
					valstk[ n_valstk - 1 ].v_value.v_ival;
			}else if( l_type == T_STRING ){
				errormsg( 1,
				"eval: '-' not defined for strings." );
			}else if( l_type == T_PAIR ){
				l_ps = valstk[ n_valstk - 2 ].v_value.v_pval;
				r_ps = valstk[ n_valstk - 1 ].v_value.v_pval;
				n_ps = pairop( "sub", l_ps, r_ps );
				valstk[ n_valstk - 2 ].v_value.v_pval = n_ps;
			}else if( l_type == T_POS ){
				l_pos = valstk[ n_valstk - 2 ].v_value.v_pval;
				r_pos = valstk[ n_valstk - 1 ].v_value.v_pval;
				n_pos = posop( "sub", l_pos, r_pos );
				valstk[ n_valstk - 2 ].v_value.v_pval = n_pos;
			}
			n_valstk--;
			break;
		case SYM_ASSIGN :
			ip = valstk[ n_valstk - 2 ].v_value.v_pval;
			l_type = ip->i_type;
			r_type = valstk[ n_valstk - 1 ].v_type;
			if( r_type == T_IDENT )
				r_type = loadidval( &valstk[ n_valstk - 1 ] );
			if( l_type == T_UNDEF )
				ip->i_type = r_type;
			else if( l_type != r_type ){
				if( l_type == T_POS && r_type == T_INT ){
					posop("cvt", &valstk[n_valstk-1], NULL);
				}else{
					errormsg( 1,
						"eval: type mismatch '='." );
				}
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
				errormsg( 1, "eval: type mismatch '+='." );
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
					errormsg( 1,
					"eval: can't alloc sp for str +." );
				}
				strcpy( sp, l_sp );
				strcat( sp, r_sp );
				valstk[ n_valstk - 2 ].v_value.v_pval = sp;
			}else if( l_type == T_PAIR ){
				l_ps = valstk[ n_valstk - 2 ].v_value.v_pval;
				r_ps = valstk[ n_valstk - 1 ].v_value.v_pval;
				n_ps = pairop( "add", l_ps, r_ps );
				valstk[ n_valstk - 2 ].v_value.v_pval = n_ps;
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
				errormsg( 1, "eval: type mismatch '-='." );
			}
			if( l_type == T_INT ){
				valstk[ n_valstk - 2 ].v_value.v_ival -=
					valstk[ n_valstk - 1 ].v_value.v_ival;
			}else if( l_type == T_STRING ){
				errormsg( 1,
				"eval: '-' not defined for strings." );
			}else if( l_type == T_PAIR ){
				l_ps = valstk[ n_valstk - 2 ].v_value.v_pval;
				r_ps = valstk[ n_valstk - 1 ].v_value.v_pval;
				n_ps = pairop( "sub", l_ps, r_ps );
				valstk[ n_valstk - 2 ].v_value.v_pval = n_ps;
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
	int	i;
	PAIRSET_T	*ps ;

	ip = vp->v_value.v_pval;
	type = ip->i_type;
	if( type == T_INT ){
		if( ip->i_val.v_value.v_ival == UNDEF ){
			sprintf( emsg,
				"loadidval: id '%s' has int value UNDEF.",
				ip->i_name );
			errormsg( 1, emsg );
		}
		vp->v_type = T_INT;
		vp->v_value.v_ival = ip->i_val.v_value.v_ival;
	}else if( type == T_STRING ){
		if( ip->i_val.v_value.v_pval == NULL ){
			sprintf( emsg,
				"loadidval: id '%s' has string value NULL.",
				ip->i_name );
			errormsg( 1, emsg );
		}
		sp = ( char * )malloc( strlen( ip->i_val.v_value.v_pval ) + 1 );
		if( sp == NULL ){
			errormsg( 1, "loadidval: can't allocate sp.",
				ip->i_name );
		}
		vp->v_type = T_STRING;
		strcpy( sp, ip->i_val.v_value.v_pval );
		vp->v_value.v_pval = sp;
	}else if( type == T_PAIR ){
		if( ip->i_val.v_value.v_pval == NULL ){
			if( def_pairset != NULL )
				ps = def_pairset;
			else{
				sprintf( emsg,
				"loadidval: id '%s' has pair value NULL.",
					 ip->i_name );
				errormsg( 1, emsg );
			}
		}else
			ps = ip->i_val.v_value.v_pval;
		vp->v_type = T_PAIR;
		vp->v_value.v_pval = pairop( "copy", ps, NULL );
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
		ip->i_val.v_type = T_INT;
		ip->i_val.v_value.v_ival = vp->v_value.v_ival;
	}else if( type == T_STRING ){
		ip->i_type = T_STRING;
		sp = ( char * )malloc( strlen( vp->v_value.v_pval ) + 1 );
		if( sp == NULL ){
			errormsg( 1, "storeexprval: can't allocate sp." );
		}
		strcpy( sp, vp->v_value.v_pval ); 
		ip->i_val.v_type = T_STRING;
		ip->i_val.v_value.v_pval = sp;
	}else if( type == T_PAIR ){
		ip->i_type = T_PAIR;
		ip->i_val.v_type = T_PAIR;
		ip->i_val.v_value.v_pval = vp->v_value.v_pval;
	}else if( type == T_POS ){
		ip->i_type = T_POS;
		ip->i_val.v_type = T_POS;
		ip->i_val.v_value.v_pval = vp->v_value.v_pval;
	}
}

static	PAIRSET_T	*pairop( op, ps1, ps2 )
char	op[];
PAIRSET_T	*ps1;
PAIRSET_T	*ps2;
{
	int	i, j, c, b, nb, sz, diff;
	PAIRSET_T	*n_ps;
	PAIR_T	*n_pp, *pp, *ppi, *ppj;

	if( !strcmp( op, "check" ) ){
		if( ps1 == NULL ){
			errormsg( 1, "pairop: check: ps1 == NULL." );
		}
		pp = ps1->ps_pairs;
		for( nb = UNDEF, i = 0; i < ps1->ps_n_pairs; i++, pp++ ){
			b = pp->p_n_bases;
			if( nb == UNDEF )
				nb = b;
			else if( b != nb ){
				sprintf( emsg,
	"pairop: check: pairset contains elements with %d and %d bases.",
					nb, b );
				errormsg( 0, emsg );
				return( ps1 );
			}
			for( j = 0; j < b; j++ ){
				c = pp->p_bases[ j ];
				pp->p_bases[ j ] = isupper( c ) ?
					tolower( c ) : c;
			}
		}
		for( i = 0; i < ps1->ps_n_pairs - 1; i++ ){
			ppi = &ps1->ps_pairs[ i ];
			if( ppi->p_n_bases == 0 )
				continue;
			for( j = i + 1; j < ps1->ps_n_pairs; j++ ){
				ppj = &ps1->ps_pairs[ j ];
				if( ppj->p_n_bases == 0 )
					continue;
				for( diff = 0, b = 0; b < ppi->p_n_bases; b++ ){
					if( ppi->p_bases[b]!=ppj->p_bases[b]){
						diff = 1;
						break;
					}
				}
				if( !diff ){
					ppj->p_n_bases = 0;
					errormsg( 0,
		"pairop: check: pairset contains duplicate pair-strings." );
				}
			}
		}
		for( j = 0, i = 0; i < ps1->ps_n_pairs; i++ ){
			ppj = &ps1->ps_pairs[ j ];
			ppi = &ps1->ps_pairs[ i ];
			if( ppi->p_n_bases != 0 ){
				*ppj = *ppi;
				j++;
			}
		}
		ps1->ps_n_pairs = j;
		return( ps1 );
	}else if( !strcmp( op, "copy" ) ){
		if( ps1 == NULL )
			return( NULL );
		n_ps = ( PAIRSET_T * )malloc( sizeof( PAIRSET_T ) );
		if( n_ps == NULL ){
			errormsg( 1, "pairop: copy: can't allocate n_ps." );
		}
		n_pp = ( PAIR_T * )malloc( ps1->ps_n_pairs*sizeof( PAIR_T ) );
		if( n_pp == NULL ){
			errormsg( 1, "pairop: copy: can't allocate n_pp." );
		}
		n_ps->ps_n_pairs = ps1->ps_n_pairs;
		n_ps->ps_pairs = n_pp;
		for( i = 0; i < n_ps->ps_n_pairs; i++ )
			n_ps->ps_pairs[ i ] = ps1->ps_pairs[ i ];
		return( n_ps );
	}else if( !strcmp( op, "add" ) ){
		ppi = ps1->ps_pairs;
		ppj = ps2->ps_pairs;
		if( ppi->p_n_bases != ppj->p_n_bases ){
			sprintf( emsg,
			"pairop: add: pairsets have %d and %d elements.",
				ppi->p_n_bases, ppj->p_n_bases );
			errormsg( 1, emsg );
		}
		sz = ps1->ps_n_pairs + ps2->ps_n_pairs;
		n_ps = ( PAIRSET_T * )malloc( sizeof( PAIRSET_T ) );
		if( n_ps == NULL ){
			errormsg( 1, "pairop: add: can't allocate n_ps." );
		}
		n_pp = ( PAIR_T * )malloc( sz * sizeof( PAIR_T ) );
		if( n_pp == NULL ){
			errormsg( 1, "pairop: add: can't allocate n_pp." );
		}
		n_ps->ps_n_pairs = ps1->ps_n_pairs;
		n_ps->ps_pairs = n_pp;
		for( i = 0; i < n_ps->ps_n_pairs; i++ )
			n_ps->ps_pairs[ i ] = ps1->ps_pairs[ i ];
		n_pp = &n_ps->ps_pairs[ n_ps->ps_n_pairs ];
		for( j = 0; j < ps2->ps_n_pairs; j++ ){
			ppj = &ps2->ps_pairs[ j ];
			for( i = 0; i < ps1->ps_n_pairs; i++ ){
				ppi = &ps1->ps_pairs[ i ];
				for( b = 0; b < ppi->p_n_bases; b++ ){
					if( ppi->p_bases[b]!=ppj->p_bases[b]){
						*n_pp = *ppj;
						n_pp++;
						n_ps->ps_n_pairs++;
						goto ADDED;
					}
				}
			}
			ADDED : ;
		}
		return( n_ps );
	}else if( !strcmp( op, "sub" ) ){
		ppi = ps1->ps_pairs;
		ppj = ps2->ps_pairs;
		if( ppi->p_n_bases != ppj->p_n_bases ){
			sprintf( emsg,
			"pairop: sub: pairsets have %d and %d elements.",
				ppi->p_n_bases, ppj->p_n_bases );
			errormsg( 1, emsg );
		}
		sz = ps1->ps_n_pairs;
		n_ps = ( PAIRSET_T * )malloc( sizeof( PAIRSET_T ) );
		if( n_ps == NULL ){
			errormsg( 1, "pairop: add: can't allocate n_ps." );
		}
		n_pp = ( PAIR_T * )malloc( sz * sizeof( PAIR_T ) );
		if( n_pp == NULL ){
			errormsg( 1, "pairop: add: can't allocate n_pp." );
		}
		n_ps->ps_n_pairs = ps1->ps_n_pairs;
		n_ps->ps_pairs = n_pp;
		for( i = 0; i < n_ps->ps_n_pairs; i++ )
			n_ps->ps_pairs[ i ] = ps1->ps_pairs[ i ];
		for( j = 0; j < ps2->ps_n_pairs; j++ ){
			ppj = &ps2->ps_pairs[ j ];
			for( i = 0; i < n_ps->ps_n_pairs; i++ ){
				n_pp = &n_ps->ps_pairs[ i ];
				for( b = 0; b < n_pp->p_n_bases; b++ ){
					if( n_pp->p_bases[b]!=ppj->p_bases[b])
						goto NOSUB;
				}
				n_pp->p_n_bases = 0;
			}
			NOSUB : ;
		}
		for( j = 0, i = 0; i < ps1->ps_n_pairs; i++ ){
			ppj = &n_ps->ps_pairs[ j ];
			ppi = &n_ps->ps_pairs[ i ];
			if( ppi->p_n_bases != 0 ){
				*ppj = *ppi;
				j++;
			}
		}
		n_ps->ps_n_pairs = j;
		return( n_ps );
	}else{
		sprintf( emsg, "pairop: unknown op '%s'.", op );
		errormsg( 1, emsg );
		return( NULL );
	}
}

static	POS_T	*posop( op, ptr, r_pos )
char	op[];
void	*ptr;
POS_T	*r_pos;
{
	VALUE_T	*vp;
	POS_T	*n_pos, *l_pos;

	if( !strcmp( op, "cvt" ) ){
		vp = ( VALUE_T * )ptr;
		if( vp->v_value.v_ival < 0 ){
			errormsg( 1,
		"posop: cvt: only ints > 0 can be convert to positions." );
		}
		n_pos = ( POS_T * )malloc( sizeof( POS_T ) );
		if( n_pos == NULL ){
			errormsg( 1, "posop: cvt: can't alloc n_pos." );
		}
		n_pos->p_type = SYM_DOLLAR;
		n_pos->p_lineno = rm_emsg_lineno;
		n_pos->p_tag = NULL;
		n_pos->p_l2r = 1;
		n_pos->p_offset = vp->v_value.v_ival;
		vp->v_type = T_POS;
		vp->v_value.v_pval = n_pos;
		return( n_pos );
	}else if( !strcmp( op, "sub" ) ){
		l_pos = ( POS_T * )ptr;
		if( l_pos->p_l2r || !r_pos->p_l2r ){
			errormsg( 1,
"posop: sub: expr must have the form '% - expr'; expr is int. valued > 0." );
		}
		n_pos = ( POS_T * )malloc( sizeof( POS_T ) );
		if( n_pos == NULL ){
			errormsg( 1, "posop: sub: can't alloc n_pos." );
		}
		n_pos->p_type = SYM_DOLLAR;
		n_pos->p_lineno = rm_emsg_lineno;
		n_pos->p_tag = NULL;
		n_pos->p_l2r = 0;
		n_pos->p_offset = l_pos->p_offset + r_pos->p_offset;
		return( n_pos );
	}else{
		sprintf( emsg, "posop: unknown op '%s'.", op );
		errormsg( 1, emsg );
		return( NULL );
	}
}

static	void	seqlen( seq, minlen, maxlen )
char	seq[];
int	*minlen;
int	*maxlen;
{

	*minlen = strlen( seq );
	*maxlen = strlen( seq );
}

void	POS_open( ptype )
int	ptype;
{
	VALUE_T	val;
	IDENT_T	*ip;

	n_valstk = 0;
	if( rm_n_pos == rm_s_pos ){
		rm_emsg_lineno = rm_lineno;
		sprintf( emsg, "POS_open: pos array size(%d) exceeded." );
		errormsg( 1, emsg );
	}
	posp = &rm_pos[ rm_n_pos ];
	rm_n_pos++;
	posp->p_type = ptype;
	posp->p_lineno = rm_lineno;
	posp->p_tag = NULL;
	posp->p_l2r = 1;	/* 1 = 1..$; 0 = $..1	*/
	posp->p_offset = 0;

	n_local_ids = 0;
	val.v_type = T_STRING;
	val.v_value.v_pval = NULL;
	ip = enter_id( "tag", T_STRING, C_VAR, S_SITE, &val );

	val.v_type = T_POS;
	val.v_value.v_pval = NULL;
	ip = enter_id( "pos", T_POS, C_VAR, S_SITE, &val );
}

void	POS_addval( expr )
NODE_T	*expr;
{

}

void	POS_close()
{
	int	i;
	IDENT_T	*ip;
	POS_T	*i_pos;

	for( i = 0; i < n_local_ids; i++ ){
		ip = local_ids[ i ];
		if( !strcmp( ip->i_name, "tag" ) ){
			posp->p_tag = ip->i_val.v_value.v_pval;
		}else if( !strcmp( ip->i_name, "pos" ) ){
			i_pos = ip->i_val.v_value.v_pval;
			posp->p_l2r = i_pos->p_l2r;
			posp->p_offset = i_pos->p_offset;
		}
	}
}

void	SI_close()
{
	int	i;
	POS_T	*posp;

	fprintf( stderr, "SITE: %d positions.\n", rm_n_pos );
	for( posp = rm_pos, i = 0; i < rm_n_pos; i++, posp++ )
		RM_dump_pos( stderr, posp );
	rm_n_pos = 0;
}
