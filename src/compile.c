#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "rnamot.h"
#include "y.tab.h"

extern	int	rm_error;
extern	VALUE_T	rm_tokval;
extern	int	rm_lineno;
extern	int	rm_emsg_lineno;
extern	char	rm_fname[];
extern	int	rm_copt;
extern	int	rm_dopt;

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

extern	SITE_T	*rm_sites;

NODE_T	*PR_close();

static	IDENT_T	*enter_id();
IDENT_T	*find_id();
static	void	eval();
static	int	loadidval();
static	void	storeexprval();
static	PAIRSET_T	*pairop();
static	POS_T	*posop();

static	int	seqlen();
static	int	link_tags();
static	void	chk_tagorder();
static	void	mk_links();
static	void	duptags_error();
static	int	chk_proper_nesting();
static	void	find_pknots();
static	int	chk_strel_parms();
static	int	chk_1_strel_parms();
static	int	chk_len_seq();
static	int	chk_site();
static	STREL_T	*set_scopes();

int	RM_init( argc, argv )
int	argc;
char	*argv[];
{
	int	ac;
	IDENT_T	*ip;
	NODE_T	*np;

	for( ac = 1; ac < argc; ac++ ){
		if( !strcmp( argv[ ac ], "-c" ) )
			rm_copt = 1;
		else if( !strcmp( argv[ ac ], "-d" ) )
			rm_dopt = 1;
	}

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
	stp->s_proper = 0;
	stp->s_index = rm_n_descr - 1;
	stp->s_lineno = rm_lineno;
	stp->s_tag = NULL;
	stp->s_next = NULL;
	stp->s_inner = NULL;
	stp->s_mates = NULL;
	stp->s_n_mates = 0;
	stp->s_scopes = NULL;
	stp->s_n_scopes = 0;
	stp->s_scope = UNDEF;
	stp->s_minlen = UNDEF;
	stp->s_maxlen = UNDEF;
	stp->s_minglen = UNDEF;
	stp->s_maxglen = UNDEF;
	stp->s_minilen = UNDEF;
	stp->s_maxilen = UNDEF;
	stp->s_seq = NULL;
	stp->s_mismatch = 0;
	stp->s_mispair = 0;
	stp->s_pairset = NULL;
	
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

	val.v_type = T_INT;
	val.v_value.v_ival = UNDEF;
	ip = enter_id( "len", T_INT, C_VAR, S_STREL, &val );

	val.v_type = T_STRING;
	val.v_value.v_pval = NULL;
	ip = enter_id( "seq", T_STRING, C_VAR, S_STREL, &val );

	val.v_type = T_INT;
	val.v_value.v_ival = UNDEF;
	ip = enter_id( "mismatch", T_INT, C_VAR, S_STREL, &val );

	if( stype != SYM_SS ){ 
		val.v_type = T_INT;
		val.v_value.v_ival = UNDEF;
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
	int	i, s_minlen, s_maxlen, s_len;
	IDENT_T	*ip;

	s_minlen = 0;
	s_maxlen = 0;
	for( i = 0; i < n_local_ids; i++ ){
		ip = local_ids[ i ];
		if( !strcmp( ip->i_name, "tag" ) ){
			stp->s_tag = ip->i_val.v_value.v_pval;
		}else if( !strcmp( ip->i_name, "minlen" ) ){
			s_minlen = ip->i_val.v_value.v_ival != UNDEF;
			stp->s_minlen = ip->i_val.v_value.v_ival;
		}else if( !strcmp( ip->i_name, "maxlen" ) ){
			s_maxlen = ip->i_val.v_value.v_ival != UNDEF;
			stp->s_maxlen = ip->i_val.v_value.v_ival;
		}else if( !strcmp( ip->i_name, "len" ) ){
			s_len = ip->i_val.v_value.v_ival != UNDEF;
			if( s_len ){
				if( s_minlen || s_maxlen ){
					rm_emsg_lineno = stp->s_lineno;
					errormsg( 0,
				"len= can't be used with minlen=/maxlen=" );
				}else{
					stp->s_minlen=ip->i_val.v_value.v_ival;
					stp->s_maxlen=ip->i_val.v_value.v_ival;
				}
			}
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
	SITE_T	*sip;
	int	err;

	if( n_descr == 0 ){
		errormsg( 0, "SE_link: Descriptor has 0 elements." );
		return( 1 );
	}

	if( link_tags( n_descr, descr ) )
		return( 1 );

	if( chk_strel_parms( n_descr, descr ) )
		return( 1 );

	for( err = 0, sip = rm_sites; sip; sip = sip->s_next )
		err |= chk_site( sip );

	return( 0 );
}

static	int	link_tags( n_descr, descr )
int	n_descr;
STREL_T	descr[];
{
	int	i, j;
	STREL_T	*stp, *stp1, *stp2, *stp3;
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
	if( rm_error )
		return( rm_error );

	for( i = 0; i < n_descr; i++ ){
		stp = &descr[ i ];
		if( stp->s_type == SYM_SS )
			stp->s_proper = 1;
		else if( stp->s_type == SYM_H5 || stp->s_type == SYM_P5 ){
			stp1 = stp->s_mates[ 0 ];
			if( chk_proper_nesting( stp, stp1, descr ) ){
				stp->s_proper = 1;
				stp->s_mates[ 0 ]->s_proper = 1;
			}
		}else if( stp->s_type == SYM_T1 ){
			stp1 = stp->s_mates[ 0 ];
			if( !chk_proper_nesting( stp, stp1, descr ) ){
				rm_emsg_lineno = stp->s_lineno;
				errormsg( 0,
				"Triplex elements must be properly nested." );
				continue;
			}
			stp2 = stp->s_mates[ 1 ];
			if( chk_proper_nesting( stp1, stp2, descr ) ){
				stp->s_proper = 1;
				stp->s_mates[ 0 ]->s_proper = 1;
				stp->s_mates[ 1 ]->s_proper = 1;
			}
		}else if( stp->s_type == SYM_Q1 ){
			stp1 = stp->s_mates[ 0 ];
			if( !chk_proper_nesting( stp, stp1, descr ) ){
				rm_emsg_lineno = stp->s_lineno;
				errormsg( 0,
				"Quad elements must be properly nested." );
				continue;
			}
			stp2 = stp->s_mates[ 1 ];
			if( !chk_proper_nesting( stp1, stp2, descr ) ){
				rm_emsg_lineno = stp->s_lineno;
				errormsg( 0,
				"Quad elements must be properly nested." );
				continue;
			}
			stp3 = stp->s_mates[ 2 ];
			if( chk_proper_nesting( stp2, stp3, descr ) ){
				stp->s_proper = 1;
				stp->s_mates[ 0 ]->s_proper = 1;
				stp->s_mates[ 1 ]->s_proper = 1;
				stp->s_mates[ 2 ]->s_proper = 1;
			}
		}
	}
	if( rm_error )
		return( rm_error );

	for( i = 0; i < n_descr; i++ )
		descr[ i ].s_checked = 0;
	for( i = 0; i < n_descr; i++ ){
		stp = &descr[ i ];
		if( stp->s_checked )
			continue;
		if( stp->s_type == SYM_H5 )
			find_pknots( stp, n_descr, descr );
	}
	if( rm_error )
		return( rm_error );

	set_scopes( 0, n_descr - 1, descr );

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
	STREL_T	**stpm;
	STREL_T	**stps;

	for( i = 0; i < n_tags; i++ ){
		stpm = ( STREL_T ** )malloc(( n_tags-1 )* sizeof( STREL_T * ));
		for( k = 0, j = 0; j < n_tags; j++ ){
			if( j != i ){
				stpm[ k ] = tags[ j ];
				k++;
			}
		}
		tags[ i ]->s_mates = stpm;
		tags[ i ]->s_n_mates = n_tags - 1;

		stps = ( STREL_T ** )malloc(( n_tags )* sizeof( STREL_T * ));
		for( j = 0; j < n_tags; j++ ){
			stps[ j ] = tags[ j ];
		}
		tags[ i ]->s_scopes = stps;
		tags[ i ]->s_n_scopes = n_tags;
		tags[ i ]->s_scope = i;
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

static	int	chk_proper_nesting( stp0, stp1, descr )
STREL_T	*stp0;
STREL_T	*stp1;
STREL_T	descr[];
{
	int	i, i0, i1, j;
	STREL_T	*stp, *stpj;

	i0 = stp0->s_index;
	i1 = stp1->s_index;
	for( i = i0 + 1; i < i1; i++ ){
		stp = &descr[ i ];
		for( j = 0; j < stp->s_n_mates; j++ ){
			stpj = stp->s_mates[ j ];
			if( stpj->s_index < i0 || stpj->s_index > i1 )
				return( 0 );
		}
	}
	return( 1 );
}

static	void	find_pknots( stp, n_descr, descr )
STREL_T	*stp;
int	n_descr;
STREL_T	descr[];
{
	int	i, j, k;
	int	pk, h5, h3;
	STREL_T	*stp1, *stp2, *stp3;
	STREL_T	*pknot[ 4 ];
	STREL_T	**stps;

	if( stp->s_type == SYM_SS ){
		stp->s_checked = 1;
		return;
	}
	if( stp->s_proper ){
		stp->s_checked = 1;
		for( j = 0; j < stp->s_n_mates; j++ ){
			stp1 = stp->s_mates[ j ];
			stp1->s_checked = 1;
		}
		return;
	}

	/* improper structure: only pknots permitted:	*/
	stp2 = stp->s_mates[ 0 ];
	for( pk = 0, j = stp->s_index + 1; j < n_descr; j++ ){
		stp1 = &descr[ j ];
		if( stp1->s_type != SYM_H5 )
			continue;
		stp3 = stp1->s_mates[ 0 ];
		if( stp3->s_index > stp2->s_index ){
			stp1->s_checked = 1;
			stp2->s_checked = 1;
			stp3->s_checked = 1;
			pk = 1;
			break;
		}
	}

	if( !pk ){
		rm_emsg_lineno = stp->s_lineno;
		sprintf( emsg, "fpk: INTERNAL ERROR: improper helix %d.",
			stp->s_index );
		errormsg( 1, emsg );
	}

	pknot[ 0 ] = stp;
	pknot[ 1 ] = stp1;
	pknot[ 2 ] = stp2;
	pknot[ 3 ] = stp3;
	for( i = 0; i < 3; i++ ){
		h5 = pknot[ i ]->s_index;
		h3 = pknot[ i + 1 ]->s_index;
		for( j = h5 + 1; j < h3; j++ ){
			stp1 = &descr[ j ];
			for( k = 0; k < stp1->s_n_mates; k++ ){
				stp2 = stp1->s_mates[ k ];
				if( stp2->s_index < h5 || stp2->s_index > h3 ){
					rm_emsg_lineno = pknot[i]->s_lineno;
					errormsg( 0, "improper pseudoknot." );
					return;
				}
			}
		}
	}

	if( rm_error )
		return;

	for( i = 0; i < 4; i++ ){
		stps = ( STREL_T ** )malloc( 4 * sizeof( STREL_T * ) );
		if( stps == NULL )
			errormsg( 1, "find_pknot: can't alloc stps." );
		for( j = 0; j < 4; j++ )
			stps[ j ] = pknot[ j ];
		free( pknot[i]->s_scopes );
		pknot[i]->s_scopes = stps;
		pknot[i]->s_n_scopes = 4;
		pknot[i]->s_scope = i;
	}
}

static	int	chk_strel_parms( n_descr, descr )
int	n_descr;
STREL_T	descr[];
{
	int	i;
	STREL_T	*stp;
	int	err;

	for( err = 0, stp = descr, i = 0; i < n_descr; i++, stp++ )
		err |= chk_1_strel_parms( stp );
	return( err );
}

static	int	chk_1_strel_parms( stp )
STREL_T	*stp;
{
	int	err;
	int	stype;
	STREL_T	*egroup[ 4 ];
	int	i, n_egroup;
	STREL_T	*stp1, *stpv;
	int	ival;
	PAIRSET_T	*pval;
	IDENT_T	*ip;

	err = 0;

	/* all elements:	*/
	if( stp->s_mismatch == UNDEF )
		stp->s_mismatch = 0;

	stype = stp->s_type;

	/* all other parms of these elements are checked during the	*/
	/* the checking of the associated "starting" element		*/
	if(	stype == SYM_P3 ||
		stype == SYM_H3 ||
		stype == SYM_T2 || stype == SYM_T3 ||
		stype == SYM_Q2 || stype == SYM_Q3 || stype == SYM_Q4 )
	{
		return( err );
	}

	/* assemble a group of related elements. ss is a trivial group	*/
	/* with 1 member						*/
	egroup[ 0 ] = stp;
	for( i = 0; i < stp->s_n_mates; i++ )
		egroup[ i + 1 ] = stp->s_mates[ i ];
	n_egroup = stp->s_n_mates + 1;

	/* check & set the lengths from minlen, maxlen & seq		*/ 
	if(	stype == SYM_SS || stype == SYM_P5 || stype == SYM_H5 ||
		stype == SYM_T1 || stype == SYM_Q1 )
	{
		err = chk_len_seq( n_egroup, egroup ); 
	}

	/* check & set the mispair & pair values	*/
	if(	stype == SYM_P5 || stype == SYM_H5 ||
		stype == SYM_T1 || stype == SYM_Q1 )
	{
		for( stpv = NULL, i = 0; i < n_egroup; i++ ){
			stp1 = egroup[ i ];
			if( stp1->s_mispair != UNDEF ){
				if( stpv == NULL )
					stpv = stp1;
				else if( stpv->s_mispair != stp1->s_mispair ){
					err = 1;
					rm_emsg_lineno = stp1->s_lineno;
					errormsg( 0,
					"inconsistant mispair values." );
				}
			}
		}
		if( !err ){
			ival = stpv ? stpv->s_mispair : 0;
			for( i = 0; i < n_egroup; i++ ){
				stp1 = egroup[ i ];
				stp1->s_mispair = ival;
			}
		}

		for( stpv = NULL, i = 0; i < n_egroup; i++ ){
			stp1 = egroup[ i ];
			if( stp1->s_pairset != NULL ){
				if( stpv == NULL )
					stpv = stp1;
				else if( !pairop( "equal", stpv->s_pairset,
					stp1->s_pairset ) ){
					err = 1;
					rm_emsg_lineno = stp1->s_lineno;
					errormsg( 0,
					"inconsistant pairset values." );
				}
			}
		}
		if( stpv == NULL ){
			if( stype == SYM_P5 || stype == SYM_H5 ){
				ip = find_id( "wc" );
				pval = ip->i_val.v_value.v_pval;
			}else if( stype == SYM_T1 ){
				ip = find_id( "tr" );
				pval = ip->i_val.v_value.v_pval;
			}else if( stype == SYM_Q1 ){
				ip = find_id( "qu" );
				pval = ip->i_val.v_value.v_pval;
			}
		}else
			pval = stpv->s_pairset;
		if( !err ){
			for( i = 0; i < n_egroup; i++ ){
				stp1 = egroup[ i ];
				if( stp1->s_pairset == NULL )
					stp1->s_pairset = pairop( "copy",
						pval, NULL );
			}
		}
	}

	return( err );
}

static	int	chk_len_seq( n_egroup, egroup )
int	n_egroup;
STREL_T	*egroup[];
{
	int	err, exact, exact1, inexact, seq;
	int	i;
	int	minl, maxl;
	int	se_minl, se_maxl;
	int	si_minl, si_maxl;
	int	s_minl, s_maxl;
	STREL_T	*stp;

	err = 0;
	for( minl = UNDEF, i = 0; i < n_egroup; i++ ){
		stp = egroup[ i ];
		if( stp->s_minlen != UNDEF ){
			if( minl == UNDEF )
				minl = stp->s_minlen;
			else if( stp->s_minlen != minl ){
				err = 1;
				rm_emsg_lineno = stp->s_lineno;
				errormsg( 0, "inconsistant minlen values." );
			}
		}
	}

	for( maxl = UNDEF, i = 0; i < n_egroup; i++ ){
		stp = egroup[ i ];
		if( stp->s_maxlen != UNDEF ){
			if( maxl == UNDEF )
				maxl = stp->s_maxlen;
			else if( stp->s_maxlen != maxl ){
				err = 1;
				rm_emsg_lineno = stp->s_lineno;
				errormsg( 0, "inconsistant maxlen values." );
			}
		}
	}

	se_minl = se_maxl = UNDEF;
	si_minl = si_maxl = UNDEF;
	exact = 0;
	inexact = 0;
	seq = 0;
	for( i = 0; i < n_egroup; i++ ){
		stp = egroup[ i ];
		if( seqlen( stp->s_seq, &s_minl, &s_maxl, &exact1 ) ){
			seq = 1;
			if( exact1 ){
				if( se_minl == UNDEF ){
					exact = 1;
					se_minl = s_minl;
					se_maxl = s_maxl;
				}else{
					if( s_minl != se_minl ){
						err = 1;
						rm_emsg_lineno = stp->s_lineno;
						errormsg( 0,
						"inconsistant seq lengths." );
					}
					if( s_maxl != se_maxl ){
						err = 1;
						rm_emsg_lineno = stp->s_lineno;
						errormsg( 0,
						"inconsistant seq lengths." );
					}
				}
			}else{
				inexact = 1;
				if( si_minl == UNDEF ){
					si_minl = s_minl;
					si_maxl = s_maxl;
				}else{
					if( s_minl > si_minl )
						si_minl = s_minl;
					if( s_maxl > si_maxl )
						si_maxl = s_maxl;
				}
			}
		}
	}

	if( exact ){
		if( inexact ){
			if( si_minl > se_maxl ){
				err = 1;
				rm_emsg_lineno = egroup[ 0 ]->s_lineno;
				errormsg( 0, "inconsistant seq lengths." );
			}
		}
		if( minl == UNDEF )
			minl = se_minl;
		else if( minl != se_minl ){
			err = 1;
			rm_emsg_lineno = egroup[ 0 ]->s_lineno;
			errormsg( 0, "inconsistant seq & minlen parms." );
		}
		if( maxl == UNDEF )
			maxl = se_maxl;
		else if( maxl != se_maxl ){
			err = 1;
			rm_emsg_lineno = egroup[ 0 ]->s_lineno;
			errormsg( 0, "inconsistant seq & maxlen parms." );
		}
	}else if( inexact ){
		if( minl == UNDEF )
			minl = si_minl;
		if( maxl == UNDEF )
			maxl = si_maxl;
		else if( maxl < si_minl ){
			err = 1;
			rm_emsg_lineno = egroup[ 0 ]->s_lineno;
			errormsg( 0, "inconsistant seq & maxlen parms." );
		}
	}

	if( minl == UNDEF )
		minl = 1;
	if( maxl == UNDEF )
		maxl = UNBOUNDED;

	if( !err ){
		for( i = 0; i < n_egroup; i++ ){
			stp = egroup[ i ];
			stp->s_minlen = minl;
			stp->s_maxlen = maxl;
		}
	}

	return( err );
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

IDENT_T	*find_id( name )
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
	int	i, j, c, b, nb, sz, diff, fnd;
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
	}else if( !strcmp( op, "equal" ) ){
		if( ps1->ps_n_pairs != ps2->ps_n_pairs )
			return( NULL );
		ppi = ps1->ps_pairs;
		ppj = ps2->ps_pairs;
		if( ppi->p_n_bases != ppj->p_n_bases )
			return( NULL );
		for( i = 0; i < ps1->ps_n_pairs; i++, ppi++ ){
			ppj = ps2->ps_pairs;
			for( j = 0; j < ps2->ps_n_pairs; j++, ppj++ ){
				for( fnd = 1, b = 0; b < ppi->p_n_bases; b++ ){
					if( ppi->p_bases[b]!=ppj->p_bases[b] ){
						fnd = 0;
						break;
					}
				}
				if( fnd )
					break;
			}
			if( !fnd )
				return( NULL );
		}
		return( ps1 );
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

static	int	seqlen( seq, minlen, maxlen, exact )
char	seq[];
int	*minlen;
int	*maxlen;
int	*exact;
{
	char	*sp, *sp1;
	int	rbr;
	int	minl, maxl;
	int	circ, dollar;

	/* no string! */
	if( seq == NULL || *seq == '\0' )
		return( 0 );

	/* useful exception to strict rules of RE*	*/
	if( *seq == '*' ){
		*minlen = 0;
		*maxlen = UNBOUNDED;
		return( 1 );
	}

	circ = 0;
	dollar = 0;
	minl = 0;
	maxl = 0;
	sp = seq;
	if( *sp == '^' ){	/* leading ^ anchors to position 1 */
		circ = 1;
		sp++;
	}
	while( *sp ){
		if( *sp == '.' ){
			if( sp[ 1 ] == '*' ){
				maxl = UNBOUNDED;
				sp++;
			}else{
				minl++;
				if( maxl != UNBOUNDED )
					maxl++;
			}
		}else if( *sp == '[' ){
			sp++;
			if( *sp == '^' )
				sp++;
			if( *sp == ']' )
				sp++;
			for( rbr = 0; *sp; sp++ ){
				if( *sp == ']' ){
					rbr = 1;
					break;
				}
			}
			if( !rbr ){
				sprintf( emsg,
					"unclosed char class in pat '%s'",
					seq );
				errormsg( 0, emsg );
				return( 0 );
			}
			if( sp[ 1 ] == '*' ){
				maxl = UNBOUNDED;
				sp++;
			}else{
				minl++;
				if( maxl != UNBOUNDED )
					maxl++;
			}
		}else if( *sp == '$' ){
			if( sp[ 1 ] == '*' ){
				maxl = UNBOUNDED;
				sp++;
			}else if( sp[ 1 ] != '\0' ){
				minl++;
				if( maxl != UNBOUNDED )
					maxl++;
			}else
				dollar = 1;
		}else if( *sp == '\\' ){
			if( sp[ 1 ] != '(' && sp[ 1 ] != ')' ){
				minl++;
				if( maxl != UNBOUNDED )
					maxl++;
			}
			sp++;
		}else if( sp[ 1 ] == '*' ){
			maxl = UNBOUNDED;
			sp++;
		}else{
			minl++;
			if( maxl != UNBOUNDED )
				maxl++;
		}
		sp++;
	}
	*minlen = minl;
	if( circ && dollar )
		*maxlen = maxl;
	else
		*maxlen = UNBOUNDED;
	*exact = circ && dollar;
	return( 1 );
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
	posp->p_dindex = UNDEF;
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

void	POS_close( parms )
int	parms;
{
	int	i;
	IDENT_T	*ip;
	POS_T	*i_pos;

	if( !parms ){
		errormsg( 1,
			"POS_close: site descr. requires pos parameter." );
	}
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

void	SI_close( expr )
NODE_T	*expr;
{
	int	i;
	POS_T	*posp;
	PAIRSET_T	*ps;
	SITE_T	*sip, *sip1;

	posp = ( POS_T * )malloc( rm_n_pos * sizeof( POS_T ) );
	if( posp == NULL ){
		rm_emsg_lineno = rm_pos[ 0 ].p_lineno;
		errormsg( 1, "SI_close: can't allocate posp." );
	}
	for( i = 0; i < rm_n_pos; i++ )
		posp[ i ] = rm_pos[ i ];
	sip = ( SITE_T * )malloc( sizeof( SITE_T ) );
	if( sip == NULL ){
		rm_emsg_lineno = rm_pos[ 0 ].p_lineno;
		errormsg( 1, "SI_close: can't allocate sip." );
	}
	sip->s_next = NULL;
	sip->s_pos = posp;
	sip->s_n_pos = rm_n_pos;
	ps = expr->n_val.v_value.v_pval;
	sip->s_pairset = pairop( "copy", ps, NULL );

	if( rm_sites == NULL )
		rm_sites = sip;
	else{
		for( sip1 = rm_sites; sip1->s_next; sip1 = sip1->s_next)
			;
		sip1->s_next = sip;
	}

	rm_n_pos = 0;
}

static	int	chk_site( sip )
SITE_T	*sip;
{
	int	err;
	int	i, j;
	POS_T	*posp;
	char	*sp;
	STREL_T	*stp;

	err = 0;
	if( sip->s_n_pos != sip->s_pairset->ps_pairs[ 0 ].p_n_bases ){
		err = 1;
		rm_emsg_lineno = sip->s_pos[ 0 ].p_lineno;
		errormsg( 0,
	"chk_site: Number of positions in site must agree with pairset." );
	}
	for( posp = sip->s_pos, i = 0; i < sip->s_n_pos; i++, posp++ ){
		if( ( sp = posp->p_tag ) == NULL ){
			err = 1;
			rm_emsg_lineno = posp->p_lineno;
			errormsg( 0, "all positions must be tagged." );
		}else{
			stp = rm_descr;
			for( j = 0; j < rm_n_descr; j++, stp++ ){
				if( stp->s_tag == NULL )
					continue;
				if( stp->s_type != posp->p_type )
					continue;
				if( !strcmp( sp, stp->s_tag ) ){
					posp->p_dindex = stp->s_index;
					break;
				}
			}
			if( posp->p_dindex == UNDEF ){
				err = 1;
				sprintf( emsg,
					"position with undefined tag '%s'.",
					sp );
				rm_emsg_lineno = posp->p_lineno;
				errormsg( 0, emsg );
			}
		}
	}
	for( posp = sip->s_pos, i = 0; i < sip->s_n_pos; i++, posp++ ){
		if( posp->p_dindex == UNDEF )
			continue;
		stp = &rm_descr[ posp->p_dindex ];
		if( posp->p_l2r ){
			if( posp->p_offset > stp->s_minlen ){
				err = 1;
				rm_emsg_lineno = posp->p_lineno;
				errormsg( 0,
					"position offset > strel minlen." );
			}
		}else if( posp->p_offset + 1 > stp->s_minlen ){
			err = 1;
			rm_emsg_lineno = posp->p_lineno;
			errormsg( 0, "position offset > strel minlen." );
		}
	}

	return( err );
}

static	STREL_T	*set_scopes( fd, ld, descr )
int	fd;
int	ld;
STREL_T	descr[];
{
	int	d, nd, s;
	int	fd1, ld1;
	STREL_T	*stp, *stp1, *stp2;

	for( d = fd; d <= ld; d = nd ){
		stp = &descr[ d ];
		if( stp->s_n_scopes == 0 ){
			nd = d + 1;
			if( nd <= ld )
				stp->s_next = &descr[ nd ];
			continue;
		}
		for( s = 0; s < stp->s_n_scopes - 1; s++ ){
			stp1 = stp->s_scopes[ s ];
			stp2 = stp->s_scopes[ s + 1 ];
			fd1 = stp1->s_index + 1;
			ld1 = stp2->s_index - 1;
			stp1->s_inner = set_scopes( fd1, ld1, descr );
		}
		stp1 = stp->s_scopes[ stp->s_n_scopes - 1 ];
		nd = stp1->s_index + 1;
		if( nd <= ld )
			stp->s_next = &descr[ nd ];
	}
	return( &descr[ fd ] );
}
