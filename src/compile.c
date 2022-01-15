#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

double	atof();

#include "log.h"
#include "rmdefs.h"
#include "rnamot.h"
#include "mm_regexp.h"
#include "y.tab.h"

ARGS_T	*rm_args;

int	rm_context = CTX_PARMS;
VALUE_T	rm_tokval;

char	*rm_wdfname;
int	rm_preprocess = TRUE;
int	rm_unlink_xdf = TRUE;
int	rm_lineno;
int	rm_error;
int	rm_emsg_lineno;

FILE	*rm_dbfp = NULL;

#define	VALSTKSIZE	20
static	VALUE_T	valstk[ VALSTKSIZE ];
static	int	n_valstk;

IDENT_T	*rm_global_ids;
int	rm_n_global_ids = 0;

#define	LOCAL_IDS_SIZE	20
static	IDENT_T	*local_ids[ LOCAL_IDS_SIZE ];
static	int	n_local_ids;

#define	ISBASE(b)	((b)=='a'||(b)=='c'||(b)=='g'||(b)=='t'||(b)=='u'|| \
			 (b)=='A'||(b)=='C'||(b)=='G'||(b)=='T'||(b)=='U')

#define	RM_R2L(st)	\
	((st)==SYM_P3||(st)==SYM_H3||(st)==SYM_T2||(st)==SYM_Q2||(st)==SYM_Q4)

#define	CURPAIR_SIZE	20
static	char	*curpair[ CURPAIR_SIZE ];
int	n_curpair;

#define	RM_DESCR_SIZE	100
STREL_T	rm_descr[ RM_DESCR_SIZE ];
int	rm_s_descr = RM_DESCR_SIZE;
int	rm_n_descr;
int	rm_dminlen;		/* min len. of entire motif	*/
int	rm_dmaxlen;		/* max len. of entire motif	*/
STREL_T	*rm_o_stp = NULL;	/* "optimized" sub-query	*/
char	*rm_o_expbuf = NULL;	/* RE buf for optimized stp	*/
int	rm_o_lctx = UNDEF;	/* required #nt 5' of o_stp	*/
int	rm_o_rctx = UNDEF;	/* required #nt 3' of o_stp	*/
STREL_T	*rm_lctx = NULL;	/* left context			*/
int	rm_lctx_explicit = FALSE;	/* set via a ctx element	*/
STREL_T	*rm_rctx = NULL;	/* left context 		*/
int	rm_rctx_explicit = FALSE;	/* set via a ctx element	*/
static	STREL_T	*open_stp;
static	PAIRSET_T	*open_pairset = NULL;
#define	SCOPE_STK_SIZE	100
static	STREL_T	*scope_stk[ SCOPE_STK_SIZE ];
static	int	t_scope_stk;

#define	RM_POS_SIZE	10
POS_T	rm_pos[ RM_POS_SIZE ];
int	rm_s_pos = RM_POS_SIZE;
int	rm_n_pos;
static	POS_T	*posp;

SITE_T	*rm_sites = NULL;

#define	RM_B2BC_SIZE	256
int	rm_b2bc[ RM_B2BC_SIZE ];
int	rm_s_b2bc = RM_B2BC_SIZE;
char	rm_bc2b[ N_BCODES ] = { 'a', 'c', 'g', 't', 'n' };
static	char	*rm_iupac[ RM_B2BC_SIZE ];
static	int	rm_s_iupac = RM_B2BC_SIZE;

SEARCH_T	**rm_searches;
int	rm_n_searches;

	/* stuff for efn rules:	*/
int	rm_efninit;
int	rm_efn2init;
char	rm_efndatadir[ 256 ] = "";
int	rm_efndataok;
int	rm_efn2dataok;
int	rm_l_base;
int	rm_efnds_allocated;
int	*rm_hstnum;
int	*rm_bcseq;
int	*rm_basepr;
int	rm_efnusestdbp;
PAIRSET_T	*rm_efnstdbp;

	/* values of the name, score, comp, pos, len of the cur. hit */
VALUE_T	*rm_nval;
VALUE_T	*rm_sval;
VALUE_T	*rm_cval;
VALUE_T	*rm_pval;
VALUE_T	*rm_lval;

extern	int	circf;		/* RE ^ kludge	*/
extern	void	compile(char *, register char *, char *, int);

static	IDENT_T	*enterid( IDENT_T *, IDENT_T * );
static	IDENT_T	*findid( IDENT_T *, char * );

static	void	SE_init( STREL_T *, int );
static	int	ends2attr( char [] );
static	int	strict2attr( int );
static	void	eval( NODE_T *, int );
static	int	loadidval( VALUE_T *vp );
static	void	storeexprval( IDENT_T *, VALUE_T * );
static	PAIRSET_T	*pairop( char [], PAIRSET_T *, PAIRSET_T * );
static	void	*mk_bmatp( PAIRSET_T * );
static	void	*mk_rbmatp( PAIRSET_T * );
static	POS_T	*posop( char [], void *, POS_T * );
static	int	chk_context( void );
static	int	link_tags( int, STREL_T [] );
static	void	chk_tagorder( int, STREL_T *[] );
static	void	mk_links( int, STREL_T *[] );
static	void	duptags_error( int, int, STREL_T *[] );
static	int	chk_proper_nesting( STREL_T *, STREL_T *, STREL_T [] );
#if 0
static	void	find_pknots0( STREL_T *, int, STREL_T [] );
static	void	find_pknots1( STREL_T *, int, STREL_T [] );
#endif
static	void	find_pknots( STREL_T *, int, STREL_T [] );
static	int	chk_strel_parms( int, STREL_T [] );
static	int	chk_1_strel_parms( STREL_T * );
static	int	chk_len_seq( int, STREL_T *[] );
static	void	chk_strict_helices( int, STREL_T [] );
static	int	chk_site( SITE_T * );
static	STREL_T	*set_scopes( int, int, STREL_T [] );
static	void	find_gi_len( int, STREL_T [], int *, int * );
static	void	find_limits( int, STREL_T [] );
static	void	find_1_limit( STREL_T *, STREL_T [] );
static	void	find_start( STREL_T *, STREL_T [] );
static	void	find_stop( STREL_T *, STREL_T [] );
static	int	closes_unbnd( STREL_T *, STREL_T [] );
static	int	min_prefixlen( STREL_T *, STREL_T [] );
static	int	max_prefixlen( STREL_T *, STREL_T [] );
static	int	min_suffixlen( STREL_T *, STREL_T [] );
static	void	find_search_order( int, STREL_T [] );
static	void	set_search_order_links( int, SEARCH_T *[]);
static	void	optimize_query( void );
#if 0
static	void	dump_bestpat();
#endif

int	RM_init( int argc, char *argv[] )
{
	int	i;
	NODE_T	*np;
	VALUE_T	val;
	IDENT_T	*ip;

	if( ( rm_args = RM_getargs( argc, argv, FALSE ) ) == NULL )
		return( 1 );

	if( rm_args->a_vopt && !rm_args->a_sopt )
		return( 0 );

	if( rm_args->a_dfname == NULL && rm_args->a_xdfname == NULL ){
		if( !rm_args->a_sopt ){
			fprintf( stderr, U_MSG_S, argv[ 0 ] );
			return( 1 );
		}
	}else if( rm_args->a_xdfname != NULL ){
		rm_unlink_xdf = FALSE;
		rm_preprocess = rm_args->a_dfname != NULL;
	}

	for( i = 0; i < rm_s_b2bc; i++ )
		rm_b2bc[ i ] = BCODE_N;
	rm_b2bc[ 'a' ] = BCODE_A; rm_b2bc[ 'A' ] = BCODE_A;
	rm_b2bc[ 'c' ] = BCODE_C; rm_b2bc[ 'C' ] = BCODE_C;
	rm_b2bc[ 'g' ] = BCODE_G; rm_b2bc[ 'G' ] = BCODE_G;
	rm_b2bc[ 't' ] = BCODE_T; rm_b2bc[ 'T' ] = BCODE_T;
	rm_b2bc[ 'u' ] = BCODE_T; rm_b2bc[ 'U' ] = BCODE_T;
	rm_b2bc[ 'n' ] = BCODE_N; rm_b2bc[ 'N' ] = BCODE_N;

	for( i = 0; i < rm_s_iupac; i++ )
		rm_iupac[ i ] = NULL;
	rm_iupac[ 'a' ] ="a";	   rm_iupac[ 'A' ] ="a";
	rm_iupac[ 'b' ] ="[cgt]";  rm_iupac[ 'B' ] ="[cgt]";
	rm_iupac[ 'c' ] ="c";	   rm_iupac[ 'C' ] ="c";
	rm_iupac[ 'd' ] ="[agt]";  rm_iupac[ 'D' ] ="[agt]";
	rm_iupac[ 'g' ] ="g";	   rm_iupac[ 'G' ] ="g";
	rm_iupac[ 'h' ] ="[act]";  rm_iupac[ 'H' ] ="[act]";
	rm_iupac[ 'k' ] ="[gt]";   rm_iupac[ 'K' ] ="[gt]";
	rm_iupac[ 'm' ] ="[ac]";   rm_iupac[ 'M' ] ="[ac]";
	rm_iupac[ 'n' ] ="[acgt]"; rm_iupac[ 'N' ] ="[acgt]";
	rm_iupac[ 'r' ] ="[ag]";   rm_iupac[ 'R' ] ="[ag]";
	rm_iupac[ 's' ] ="[cg]";   rm_iupac[ 'S' ] ="[cg]";
	rm_iupac[ 't' ] ="t";	   rm_iupac[ 'T' ] ="t";
	rm_iupac[ 'u' ] ="t";	   rm_iupac[ 'U' ] ="t";
	rm_iupac[ 'v' ] ="[acg]";  rm_iupac[ 'V' ] ="[acg]";
	rm_iupac[ 'w' ] ="[at]";   rm_iupac[ 'W' ] ="[at]";
	rm_iupac[ 'y' ] ="[ct]";   rm_iupac[ 'Y' ] ="[ct]";

	rm_lineno = 0;
	curpair[0] = "a:u";
	curpair[1] = "c:g";
	curpair[2] = "g:c";
	curpair[3] = "u:a";
	n_curpair = 4;
	np = PR_close();
	RM_enter_id( "wc", T_PAIRSET, C_VAR, S_GLOBAL, FALSE, &np->n_val );

	curpair[0] = "g:u";
	curpair[1] = "u:g";
	n_curpair = 2;
	np = PR_close();
	RM_enter_id( "gu", T_PAIRSET, C_VAR, S_GLOBAL, FALSE, &np->n_val );

	curpair[0] = "a:u:u";
	n_curpair = 1;
	np = PR_close();
	RM_enter_id( "tr", T_PAIRSET, C_VAR, S_GLOBAL, FALSE, &np->n_val );

	curpair[0] = "g:g:g:g";
	n_curpair = 1;
	np = PR_close();
	RM_enter_id( "qu", T_PAIRSET, C_VAR, S_GLOBAL, FALSE, &np->n_val );

	val.v_type = T_INT;
	val.v_value.v_ival = 1;
	RM_enter_id( "chk_both_strs", T_INT, C_VAR, S_GLOBAL, FALSE, &val );

	val.v_type = T_INT;
	val.v_value.v_ival = 1;
	RM_enter_id( "iupac", T_INT, C_VAR, S_GLOBAL, FALSE, &val );

	val.v_type = T_INT;
	val.v_value.v_ival = 0;
	RM_enter_id( "show_progress", T_INT, C_VAR, S_GLOBAL, FALSE, &val );

	val.v_type = T_INT;
	val.v_value.v_ival = 10000;
	RM_enter_id( "ALL", T_INT, C_VAR, S_GLOBAL, FALSE, &val );

	val.v_type = T_INT;
	val.v_value.v_ival = CTX_MINLEN;
	RM_enter_id( "ctx_minlen", T_INT, C_VAR, S_GLOBAL, FALSE, &val );

	val.v_type = T_INT;
	val.v_value.v_ival = CTX_MAXLEN;
	RM_enter_id( "ctx_maxlen", T_INT, C_VAR, S_GLOBAL, FALSE, &val );

	val.v_type = T_INT;
	val.v_value.v_ival = SS_MINLEN;
	RM_enter_id( "ss_minlen", T_INT, C_VAR, S_GLOBAL, FALSE, &val );

	val.v_type = T_INT;
	val.v_value.v_ival = SS_MAXLEN;
	RM_enter_id( "ss_maxlen", T_INT, C_VAR, S_GLOBAL, FALSE, &val );

	val.v_type = T_INT;
	val.v_value.v_ival = WC_MINLEN;
	RM_enter_id( "wc_minlen", T_INT, C_VAR, S_GLOBAL, FALSE, &val );

	val.v_type = T_INT;
	val.v_value.v_ival = WC_MAXLEN;
	RM_enter_id( "wc_maxlen", T_INT, C_VAR, S_GLOBAL, FALSE, &val );

	val.v_type = T_STRING;
	val.v_value.v_pval = WC_ENDS;
	RM_enter_id( "wc_ends", T_STRING, C_VAR, S_GLOBAL, FALSE, &val );

	val.v_type = T_INT;
	val.v_value.v_ival = rm_args->a_strict_helices;
	RM_enter_id( "wc_strict", T_INT, C_VAR, S_GLOBAL, FALSE, &val );

	val.v_type = T_INT;
	val.v_value.v_ival = PHLX_MINLEN;
	RM_enter_id( "phlx_minlen", T_INT, C_VAR, S_GLOBAL, FALSE, &val);

	val.v_type = T_INT;
	val.v_value.v_ival = PHLX_MAXLEN;
	RM_enter_id( "phlx_maxlen", T_INT, C_VAR, S_GLOBAL, FALSE, &val);

	val.v_type = T_STRING;
	val.v_value.v_pval = PHLX_ENDS;
	RM_enter_id( "phlx_ends", T_STRING, C_VAR, S_GLOBAL, FALSE, &val );

	val.v_type = T_INT;
	val.v_value.v_ival = rm_args->a_strict_helices;
	RM_enter_id( "phlx_strict", T_INT, C_VAR, S_GLOBAL, FALSE, &val );

	val.v_type = T_INT;
	val.v_value.v_ival = TR_MINLEN;
	RM_enter_id( "tr_minlen", T_INT, C_VAR, S_GLOBAL, FALSE, &val );

	val.v_type = T_INT;
	val.v_value.v_ival = TR_MAXLEN;
	RM_enter_id( "tr_maxlen", T_INT, C_VAR, S_GLOBAL, FALSE, &val );

	val.v_type = T_STRING;
	val.v_value.v_pval = TR_ENDS;
	RM_enter_id( "tr_ends", T_STRING, C_VAR, S_GLOBAL, FALSE, &val );

	val.v_type = T_INT;
	val.v_value.v_ival = rm_args->a_strict_helices;
	RM_enter_id( "tr_strict", T_INT, C_VAR, S_GLOBAL, FALSE, &val );

	val.v_type = T_INT;
	val.v_value.v_ival = QU_MINLEN;
	RM_enter_id( "qu_minlen", T_INT, C_VAR, S_GLOBAL, FALSE, &val );

	val.v_type = T_INT;
	val.v_value.v_ival = QU_MAXLEN;
	RM_enter_id( "qu_maxlen", T_INT, C_VAR, S_GLOBAL, FALSE, &val );

	val.v_type = T_STRING;
	val.v_value.v_pval = QU_ENDS;
	RM_enter_id( "qu_ends", T_STRING, C_VAR, S_GLOBAL, FALSE, &val );

	val.v_type = T_INT;
	val.v_value.v_ival = rm_args->a_strict_helices;
	RM_enter_id( "qu_strict", T_INT, C_VAR, S_GLOBAL, FALSE, &val );

	val.v_type = T_INT;
	val.v_value.v_ival = 6000;
	RM_enter_id( "windowsize", T_INT, C_VAR, S_GLOBAL, FALSE, &val );

	val.v_type = T_STRING;
	val.v_value.v_pval = "";
	RM_enter_id( "efn_datadir", T_STRING, C_VAR, S_GLOBAL, FALSE, &val );

	val.v_type = T_INT;
	val.v_value.v_ival = 1;
	RM_enter_id( "efn_usestdbp", T_INT, C_VAR, S_GLOBAL, FALSE, &val );

	curpair[0] = "a:u";
	curpair[1] = "c:g";
	curpair[2] = "g:c";
	curpair[3] = "g:u";
	curpair[4] = "u:a";
	curpair[5] = "u:g";
	n_curpair = 6;
	np = PR_close();
	rm_efnstdbp = np->n_val.v_value.v_pval;
	RM_enter_id( "efn_stdbp",
		T_PAIRSET, C_VAR, S_GLOBAL, FALSE, &np->n_val );

	val.v_type = T_STRING;
	val.v_value.v_pval = NULL;
	ip = RM_enter_id( "NAME", T_STRING, C_VAR, S_GLOBAL, FALSE, &val ); 
	rm_nval = &ip->i_val;

	val.v_type = T_UNDEF;
	val.v_value.v_pval = NULL;
	ip = RM_enter_id( "SCORE", T_UNDEF, C_VAR, S_GLOBAL, TRUE, &val ); 
	rm_sval = &ip->i_val;

	val.v_type = T_INT;
	val.v_value.v_ival = 0;
	ip = RM_enter_id( "COMP", T_INT, C_VAR, S_GLOBAL, FALSE, &val ); 
	rm_cval = &ip->i_val;

	val.v_type = T_INT;
	val.v_value.v_ival = 0;
	ip = RM_enter_id( "POS", T_INT, C_VAR, S_GLOBAL, FALSE, &val ); 
	rm_pval = &ip->i_val;

	val.v_type = T_INT;
	val.v_value.v_ival = 0;
	ip = RM_enter_id( "LEN", T_INT, C_VAR, S_GLOBAL, FALSE, &val ); 
	rm_lval = &ip->i_val;

	val.v_type = T_INT;
	val.v_value.v_ival = 0;
	RM_enter_id( "NSE", T_INT, C_VAR, S_GLOBAL, FALSE, &val ); 

	val.v_type = T_INT;
	val.v_value.v_ival = 0;
	RM_enter_id( "SLEN", T_INT, C_VAR, S_GLOBAL, FALSE, &val ); 

	rm_lineno = 0;

	if( rm_args->a_sopt )
		rm_args->a_vopt = TRUE;

	RM_setprog( P_MAIN );
	
	return( 0 );
}

void	PARM_add( NODE_T *expr )
{

	n_valstk = 0;
	eval( expr, 1 );
}

void	PR_open( void )
{

	n_curpair = 0;
}

void	PR_add( NODE_T *np )
{

	if( n_curpair >= CURPAIR_SIZE ){
		rm_error = TRUE;
		LOG_ERROR("%s:%d current pair too large.", rm_wdfname, rm_lineno);
		exit(1);
	}
	curpair[ n_curpair ] = np->n_val.v_value.v_pval;
	n_curpair++;
}

NODE_T	*PR_close( void )
{
	int	i, b, needbase;
	PAIR_T	*pp;
	PAIRSET_T	*ps;
	char	*bp;
	NODE_T	*np;

	ps = ( PAIRSET_T * )malloc( sizeof( PAIRSET_T ) );
	if( ps == NULL ){
		rm_error = TRUE;
		LOG_ERROR("%s:%d can't allocate pairlist.", rm_wdfname, rm_lineno);
		exit(1);
	}
	pp = ( PAIR_T * )malloc( n_curpair * sizeof( PAIR_T ) );
	if( pp == NULL ){
		rm_error = TRUE;
		LOG_ERROR("%s:%d can't allocate pair.", rm_wdfname, rm_lineno);
		exit(1);
	}
	ps->ps_n_pairs = n_curpair;
	ps->ps_pairs = pp;
	for( pp = ps->ps_pairs, i = 0; i < ps->ps_n_pairs; i++, pp++ ){
		for( needbase = 1, b = 0, bp = curpair[ i ]; *bp; bp++ ){
			if( ISBASE( *bp ) ){
				if( needbase ){
					if( b >= 4 ){
						rm_error = TRUE;
						LOG_ERROR("%s:%d At most 4 bases in a pair-string.", rm_wdfname, rm_lineno);
						break;
					}else{
						pp->p_n_bases = b + 1;
						pp->p_bases[ b ]= *bp;
						b++;
						needbase = FALSE;
					}
				}else{
					rm_error = TRUE;
					LOG_ERROR("%s:%d pair-string is bsse-letter : base-letter : ...", rm_wdfname, rm_lineno);
					break;
				}
			}else if( *bp == ':' ){
				if( needbase ){
					rm_error = TRUE;
					LOG_ERROR("%s:%d pair-string is bsse-letter : base-letter : ...", rm_wdfname, rm_lineno);
					break;
				}
				needbase = 1;
			}else{
				rm_error = TRUE;
				LOG_ERROR("%s:%d pair-string is bsse-letter : base-letter : ...", rm_wdfname, rm_lineno);
				break;
			}
		}
		if( pp->p_n_bases < 2 || pp->p_n_bases > 4 ){
			rm_error = TRUE;
			LOG_ERROR("%s:%d pair-string has 2-4 bases", rm_wdfname, rm_lineno);
		}
	}
	ps->ps_mat[ 0 ] = NULL;
	ps->ps_mat[ 1 ] = NULL;
	ps = pairop( "check", ps, NULL );

	np = ( NODE_T * )malloc( sizeof( NODE_T ) );
	if( np == NULL ){
		rm_error = TRUE;
		LOG_ERROR("%s:%d can't allocate np.", rm_wdfname, rm_lineno);
		exit(1);
	}
	np->n_sym = SYM_PAIRSET;
	np->n_type = T_PAIRSET;
	np->n_class = C_LIT;
	np->n_lineno = rm_lineno;
	np->n_val.v_type = T_PAIRSET;
	np->n_val.v_value.v_pval = ps;
	np->n_left = NULL;
	np->n_right = NULL;
	return( np );
}

void	SE_open( int stype )
{

	n_valstk = 0;
	if( stype == SYM_SE ){
		rm_error = TRUE;
		LOG_ERROR("%s:%d strel 'se' allowed only in score section.", rm_wdfname, rm_lineno);
		exit(1);
	}
	if( stype != SYM_CTX ){
		if( rm_rctx != NULL ){
			rm_error = TRUE;
			LOG_ERROR("%s:%d Right ctx element must be last element.", rm_wdfname, rm_lineno);
			exit(1);
		}
		if( rm_n_descr == rm_s_descr ){
			rm_error = TRUE;
			LOG_ERROR("%s:%d descr array size(%d) exceeded.", rm_wdfname, rm_lineno, rm_s_descr);
			exit(1);
		}
		open_stp = &rm_descr[ rm_n_descr ];
		rm_n_descr++;
	}else if( rm_n_descr == 0 ){
		if( rm_lctx == NULL ){
			open_stp = ( STREL_T * )malloc( sizeof( STREL_T ) );
			if( open_stp == NULL ){
				rm_error = TRUE;
				LOG_ERROR("%s:%d can't allocate stp for left ctx element.", rm_wdfname, rm_lineno);
				exit(1);
			}
			rm_lctx = open_stp;
			rm_lctx_explicit = 1;
		}else{
			rm_error = TRUE;
			LOG_ERROR("%s:%d ctx elements must contain a real descriptor.", rm_wdfname, rm_lineno);
			exit(1);
		}
	}else if( rm_rctx == NULL ){
		open_stp = ( STREL_T * )malloc( sizeof( STREL_T ) );
		if( open_stp == NULL ){
			rm_error = TRUE;
			LOG_ERROR("%s:%d can't allocate stp for right ctx element.", rm_wdfname, rm_lineno);
			exit(1);
		}
		rm_rctx = open_stp;
		rm_rctx_explicit = 1;
	}else{
		rm_error = TRUE;
		LOG_ERROR("%s:%d Descr can contain at most 1 right ctx element.", rm_wdfname, rm_lineno);
		exit(1);
	}
	SE_init( open_stp, stype );
}

static	void	SE_init( STREL_T *stp, int stype )
{
	VALUE_T	val;
	IDENT_T	*ip;

	stp->s_checked = FALSE;
	stp->s_type = stype;
	stp->s_attr[ SA_PROPER ] = FALSE;
	stp->s_attr[ SA_ENDS ] = FALSE;
	stp->s_attr[ SA_STRICT ] = FALSE;
	stp->s_index = stype != SYM_CTX ? rm_n_descr - 1 : UNDEF;
	stp->s_lineno = rm_lineno;
	stp->s_searchno = UNDEF;
	stp->s_matchoff = UNDEF;
	stp->s_matchlen = UNDEF;
	stp->s_n_mismatches = UNDEF;
	stp->s_n_mispairs = UNDEF;
	stp->s_tag = NULL;
	stp->s_next = NULL;
	stp->s_prev = NULL;
	stp->s_inner = NULL;
	stp->s_outer = NULL;
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
	stp->s_start.a_l2r = FALSE;
	stp->s_start.a_offset = UNDEF;
	stp->s_stop.a_l2r = FALSE;
	stp->s_stop.a_offset = UNDEF;
	stp->s_seq = NULL;
	stp->s_expbuf = NULL;
	stp->s_e_expbuf = NULL;
	memset( &stp->s_bestpat, 0, sizeof( BESTPAT_T ) );
	stp->s_mismatch = 0;
	stp->s_matchfrac = 1.0;
	stp->s_mispair = UNDEF;
	stp->s_pairfrac = UNDEF;
	stp->s_pairset = NULL;
	
	n_local_ids = 0;
	val.v_type = T_STRING;
	val.v_value.v_pval = NULL;
	ip = RM_enter_id( "tag", T_STRING, C_VAR, S_STREL, FALSE, &val );

	val.v_type = T_INT;
	val.v_value.v_ival = UNDEF;
	ip = RM_enter_id( "minlen", T_INT, C_VAR, S_STREL, FALSE, &val );

	val.v_type = T_INT;
	val.v_value.v_ival = UNDEF;
	ip = RM_enter_id( "maxlen", T_INT, C_VAR, S_STREL, FALSE, &val );

	val.v_type = T_INT;
	val.v_value.v_ival = UNDEF;
	ip = RM_enter_id( "len", T_INT, C_VAR, S_STREL, FALSE, &val );

	val.v_type = T_STRING;
	val.v_value.v_pval = NULL;
	ip = RM_enter_id( "seq", T_STRING, C_VAR, S_STREL, FALSE, &val );

	val.v_type = T_INT;
	val.v_value.v_ival = UNDEF;
	ip = RM_enter_id( "mismatch", T_INT, C_VAR, S_STREL, FALSE, &val );

	val.v_type = T_FLOAT;
	val.v_value.v_dval = 1.0;
	ip = RM_enter_id( "matchfrac", T_FLOAT, C_VAR, S_STREL, FALSE, &val );

	if( stype != SYM_SS && stype != SYM_CTX ){ 
		stp->s_attr[ SA_ENDS ] = UNDEF;
		stp->s_attr[ SA_STRICT ] = UNDEF;
		val.v_type = T_INT;
		val.v_value.v_ival = UNDEF;
		ip = RM_enter_id( "mispair",
			T_INT, C_VAR, S_STREL, FALSE, &val );

		val.v_type = T_FLOAT;
		val.v_value.v_dval = UNDEF;
		ip = RM_enter_id( "pairfrac",
			T_FLOAT, C_VAR, S_STREL, FALSE, &val );

		val.v_type = T_STRING;
		val.v_value.v_pval = NULL;
		ip = RM_enter_id( "ends",
			T_STRING, C_VAR, S_STREL, FALSE, &val );

		val.v_type = T_INT;
		val.v_value.v_ival = UNDEF;
		ip = RM_enter_id( "strict",
			T_INT, C_VAR, S_STREL, FALSE, &val );

		switch( stype ){
		case SYM_H5 :
		case SYM_H3 :
			ip = RM_find_id( "wc" );
			open_pairset = ip->i_val.v_value.v_pval;
			break;
		case SYM_P5 :
		case SYM_P3 :
			ip = RM_find_id( "wc" );
			open_pairset = ip->i_val.v_value.v_pval;
			break;
		case SYM_T1 :
		case SYM_T2 :
		case SYM_T3 :
			ip = RM_find_id( "tr" );
			open_pairset = ip->i_val.v_value.v_pval;
			break;
		case SYM_Q1 :
		case SYM_Q2 :
		case SYM_Q3 :
		case SYM_Q4 :
			ip = RM_find_id( "qu" );
			open_pairset = ip->i_val.v_value.v_pval;
			break;
		}
		val.v_type = T_PAIRSET;
		val.v_value.v_pval = NULL;
		ip = RM_enter_id( "pair",
			T_PAIRSET, C_VAR, S_STREL, FALSE, &val );
	}
}

void	SE_addval( NODE_T *expr )
{

	n_valstk = 0;
	eval( expr, 0 );
}

void	SE_close( void )
{
	int	i, s_minlen, s_maxlen, s_len, s_mispair, s_pairfrac;
	IDENT_T	*ip;

	s_minlen = 0;
	s_maxlen = 0;
	s_mispair = 0;
	s_pairfrac = 0;
	for( i = 0; i < n_local_ids; i++ ){
		ip = local_ids[ i ];
		if( !strcmp( ip->i_name, "tag" ) ){
			open_stp->s_tag = ip->i_val.v_value.v_pval;
		}else if( !strcmp( ip->i_name, "minlen" ) ){
			s_minlen = ip->i_val.v_value.v_ival != UNDEF;
			open_stp->s_minlen = ip->i_val.v_value.v_ival;
		}else if( !strcmp( ip->i_name, "maxlen" ) ){
			s_maxlen = ip->i_val.v_value.v_ival != UNDEF;
			open_stp->s_maxlen = ip->i_val.v_value.v_ival;
		}else if( !strcmp( ip->i_name, "len" ) ){
			s_len = ip->i_val.v_value.v_ival != UNDEF;
			if( s_len ){
				if( s_minlen || s_maxlen ){
					rm_error = TRUE;
					LOG_ERROR("%s:%d len= can't be used with minlen=/maxlen=.", rm_wdfname, rm_lineno); 
				}else{
					open_stp->s_minlen= ip->i_val.v_value.v_ival;
					open_stp->s_maxlen= ip->i_val.v_value.v_ival;
				}
			}
		}else if( !strcmp( ip->i_name, "seq" ) ){
			open_stp->s_seq = ip->i_val.v_value.v_pval;
		}else if( !strcmp( ip->i_name, "mismatch" ) ){
			open_stp->s_mismatch = ip->i_val.v_value.v_ival;
		}else if( !strcmp( ip->i_name, "matchfrac" ) ){
			if( ip->i_val.v_value.v_dval < 0. || ip->i_val.v_value.v_dval > 1. ){
				rm_error = TRUE;
				LOG_ERROR("%s:%d matchfrac must be >= 0 and <= 1.", rm_wdfname, rm_lineno);
			}else
				open_stp->s_matchfrac=ip->i_val.v_value.v_dval;
		}else if( !strcmp( ip->i_name, "mispair" ) ){
			s_mispair = ip->i_val.v_value.v_ival != UNDEF;
			if(s_mispair){
				if(s_pairfrac){
					rm_error = TRUE;
					LOG_ERROR("%s:%d mispair= can't be used with pairfrac=.", rm_wdfname, rm_lineno);
				}else if(ip->i_val.v_value.v_ival < 0){
					rm_error = TRUE;
					LOG_ERROR("%s:%d bad mispair value %d, must be >= 0.", rm_wdfname, rm_lineno, ip->i_val.v_value.v_ival);
				}
			}
			open_stp->s_mispair = ip->i_val.v_value.v_ival;
		}else if( !strcmp( ip->i_name, "pairfrac" ) ){
			s_pairfrac = ip->i_val.v_value.v_dval != UNDEF;
			if( s_pairfrac ){
				if( s_mispair ){
					rm_error = TRUE;
					LOG_ERROR("%s:%d pairfrac= can't be used with mispair=.", rm_wdfname, rm_lineno);
				}else if( ip->i_val.v_value.v_dval < 0. || ip->i_val.v_value.v_dval > 1. ){
					rm_error = TRUE;
					LOG_ERROR("%s:%d pairfrac must be >= 0 and <= 1.", rm_wdfname, rm_lineno);
				}
			}
			open_stp->s_pairfrac = ip->i_val.v_value.v_dval;
		}else if( !strcmp( ip->i_name, "pair" ) ){
			open_stp->s_pairset = ip->i_val.v_value.v_pval;
		}else if( !strcmp( ip->i_name, "ends" ) ){
			if( ip->i_val.v_value.v_pval != NULL ){
				open_stp->s_attr[ SA_ENDS ] =
					ends2attr( ip->i_val.v_value.v_pval );
			}
		}else if( !strcmp( ip->i_name, "strict" ) ){
			if( ip->i_val.v_value.v_ival != UNDEF ){
				open_stp->s_attr[ SA_STRICT ] =
					strict2attr( ip->i_val.v_value.v_ival );
			}
		}
	}
	open_pairset = NULL;
	open_stp = NULL;
	n_local_ids = 0;
}

int	SE_link( int n_descr, STREL_T descr[] )
{
	SITE_T	*sip;
	int	err;

	if( n_descr == 0 ){
		rm_error = TRUE;
		LOG_ERROR("%s:%d Descriptor has 0 elements.", rm_wdfname, rm_lineno);
		return( 1 );
	}

	if( chk_context() )
		return( 1 );

	if( link_tags( n_descr, descr ) )
		return( 1 );

	if( chk_strel_parms( n_descr, descr ) )
		return( 1 );

	for( err = FALSE, sip = rm_sites; sip; sip = sip->s_next )
		err |= chk_site( sip );

	if( err )
		return( err );

	find_gi_len( 0, descr, &rm_dminlen, &rm_dmaxlen );

	find_limits( 0, descr );

	rm_searches = ( SEARCH_T ** )malloc( rm_n_descr*sizeof( SEARCH_T * ) );
	if( rm_searches == NULL ){
		rm_error = TRUE;
		LOG_ERROR("%s:%d can't allocate rm_searches.", rm_wdfname, rm_lineno);
		exit(1);
	}
	rm_n_searches = 0;
	find_search_order( 0, descr );
	set_search_order_links( rm_n_searches, rm_searches );

	if( rm_args->a_o_emin > 0.0 )
		optimize_query();

	return( err );
}

static	int	chk_context( void )
{

	if( !rm_args->a_show_context )
		return( 0 );

	if( rm_lctx == NULL ){
		rm_lctx = ( STREL_T * )malloc( sizeof( STREL_T ) );
		if( rm_lctx == NULL ){
			rm_error = TRUE;
			LOG_ERROR("%s:%d can't allocate rm_lctx.", rm_wdfname, rm_lineno);
			exit(1);
			return( 1 );
		}
		open_stp = rm_lctx;
		SE_init( open_stp, SYM_CTX );
		SE_close();
	}

	if( rm_rctx == NULL ){
		rm_rctx = ( STREL_T * )malloc( sizeof( STREL_T ) );
		if( rm_rctx == NULL ){
			rm_error = TRUE;
			LOG_ERROR("%s:%d can't allocate rm_rctx.", rm_wdfname, rm_lineno);
			exit(1);
			return( 1 );
		}
		open_stp = rm_rctx;
		SE_init( open_stp, SYM_CTX );
		SE_close();
	}

	return( 0 );
}

static	int	link_tags( int n_descr, STREL_T descr[] )
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
			rm_error = TRUE;
			LOG_ERROR("%s:%d all triple/quad. helix els. must be tagged.", rm_wdfname, rm_lineno);
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
				rm_error = TRUE;
				LOG_ERROR("%s:%d %s element has no matching %s element.", rm_wdfname, rm_lineno,
					stp->s_type == SYM_H3 ? "h3" : "p3", stp->s_type == SYM_H3 ? "h5" : "p5" );
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
			rm_error = TRUE;
			LOG_ERROR("%s:%d %s element has no matching %s element.", rm_wdfname, rm_lineno,
                                stp->s_type == SYM_H5 ? "h5" : "h3", stp->s_type == SYM_H5 ? "p5" : "p3" );
		}
	}
	if( rm_error )
		return( rm_error );

	for( i = 0; i < n_descr; i++ ){
		stp = &descr[ i ];
		if( stp->s_type == SYM_SS )
			stp->s_attr[ SA_PROPER ] = 1;
		else if( stp->s_type == SYM_H5 || stp->s_type == SYM_P5 ){
			stp1 = stp->s_mates[ 0 ];
			if( chk_proper_nesting( stp, stp1, descr ) ){
				stp->s_attr[ SA_PROPER ] = 1;
				stp->s_mates[ 0 ]->s_attr[ SA_PROPER ] = 1;
			}
		}else if( stp->s_type == SYM_T1 ){
			stp1 = stp->s_mates[ 0 ];
			if( !chk_proper_nesting( stp, stp1, descr ) ){
				rm_error = TRUE;
				LOG_ERROR("%s:%d riplex elements must be properly nested.", rm_wdfname, rm_lineno);
				continue;
			}
			stp2 = stp->s_mates[ 1 ];
			if( chk_proper_nesting( stp1, stp2, descr ) ){
				stp->s_attr[ SA_PROPER ] = 1;
				stp->s_mates[ 0 ]->s_attr[ SA_PROPER ] = 1;
				stp->s_mates[ 1 ]->s_attr[ SA_PROPER ] = 1;
			}
		}else if( stp->s_type == SYM_Q1 ){
			stp1 = stp->s_mates[ 0 ];
			if( !chk_proper_nesting( stp, stp1, descr ) ){
				rm_error = TRUE;
				LOG_ERROR("%s:%d Quad elements must be properly nested.", rm_wdfname, rm_lineno);
				continue;
			}
			stp2 = stp->s_mates[ 1 ];
			if( !chk_proper_nesting( stp1, stp2, descr ) ){
				rm_error = TRUE;
				LOG_ERROR("%s:%d Quad elements must be properly nested.", rm_wdfname, rm_lineno);
				continue;
			}
			stp3 = stp->s_mates[ 2 ];
			if( chk_proper_nesting( stp2, stp3, descr ) ){
				stp->s_attr[ SA_PROPER ] = 1;
				stp->s_mates[ 0 ]->s_attr[ SA_PROPER ] = 1;
				stp->s_mates[ 1 ]->s_attr[ SA_PROPER ] = 1;
				stp->s_mates[ 2 ]->s_attr[ SA_PROPER ] = 1;
			}
		}
	}
	if( rm_error )
		return( rm_error );

	for( i = 0; i < n_descr; i++ )
		descr[ i ].s_checked = FALSE;
	for( i = 0; i < n_descr; i++ ){
		stp = &descr[ i ];
		if( stp->s_checked )
			continue;
		if( stp->s_type == SYM_H5 )
			find_pknots( stp, n_descr, descr );
	}
	if( rm_error )
		return( rm_error );

	t_scope_stk = 0;
	scope_stk[ t_scope_stk ] = NULL;
	set_scopes( 0, n_descr - 1, descr );
	return( rm_error );
}

static	void	chk_tagorder( int n_tags, STREL_T *tags[] )
{
	int	t1, t2, t3, t4;

	t1 = tags[ 0 ]->s_type;
	if( t1 == SYM_SS ){
		if( n_tags > 1 )
			duptags_error( 1, n_tags, tags );
	}else if( t1 == SYM_H5 ){
		if( n_tags < 2 ){
			rm_error = TRUE;
			LOG_ERROR("%s:%d wc-helix '%s' has not h3() element.",
				rm_wdfname, tags[0]->s_lineno, tags[0]->s_tag);
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
			rm_error = TRUE;
			LOG_ERROR("%s:%d parallel-helix '%s' has no p3() element.",
				rm_wdfname, tags[0]->s_lineno, tags[0]->s_tag);
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
			rm_error = TRUE;
			LOG_ERROR("%s:%d triplex '%s' has < 3 elements.",
				rm_wdfname, tags[0]->s_lineno, tags[0]->s_tag);
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
			rm_error = TRUE;
			LOG_ERROR("%s:%d 4-plex '%s' has < 4 elements.",
				rm_wdfname, tags[0]->s_lineno, tags[0]->s_tag);
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
		rm_error = TRUE;
		LOG_ERROR("%s:%d 1st use of tag '%s' is out of order.",
			rm_wdfname, tags[0]->s_lineno, tags[0]->s_tag);
	}
}

static	void	mk_links( int n_tags, STREL_T *tags[] )
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

static	void	duptags_error( int need, int n_tags, STREL_T *tags[] )
{
	int	i;

	for( i = need; i < n_tags; i++ ){
		rm_error = TRUE;
		LOG_ERROR("%s:%d duplicate tag '%s'.", rm_wdfname, tags[i]->s_lineno, tags[i]->s_tag);
	}
}

static	int	chk_proper_nesting( STREL_T *stp0, STREL_T *stp1,
	STREL_T descr[] )
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

static	void	find_pknots( STREL_T *stp, int n_descr, STREL_T descr[] )
{
	int	fd, fd0, fd1;
	int	ld, ld1;
	int	d, d0, d1;
	int	diff;
	int	i, j;
	STREL_T	*stp1, *stp2, *stp3;
	static	int	*pknot = NULL;
	int	n_pknot;
	STREL_T	**stps;

	if( stp->s_type == SYM_SS ){
		stp->s_checked = 1;
		return;
	}
	if( stp->s_attr[ SA_PROPER ] ){
		stp->s_checked = 1;
		for( j = 0; j < stp->s_n_mates; j++ ){
			stp1 = stp->s_mates[ j ];
			stp1->s_checked = 1;
		}
		return;
	}

	if( pknot == NULL ){
		pknot = ( int * )malloc( n_descr * sizeof( int * ) );
		if( pknot == NULL ){
			rm_error = TRUE;
			LOG_ERROR("%s:%d can't allocate pknot.", rm_wdfname, rm_lineno);
			exit(1);
		}
	}

	stp3 = stp->s_mates[ 0 ];
	fd0 = fd = stp->s_index;
	ld = stp3->s_index; 

	for( d = 0; d < fd; d++ )
		pknot[ d ] = UNDEF; 
	for( d = fd; d < ld; d++ )
		pknot[ d ] = stp->s_index;
	pknot[ ld ] = ld;
	for( d = ld + 1; d < n_descr; d++ )
		pknot[ d ] = UNDEF;

	for( diff = TRUE; diff; ){
		for( diff = FALSE, d = fd + 1; d < ld; d++ ){
			stp1 = &rm_descr[ d ];
			if( stp1->s_type == SYM_H5 ){
				if( stp1->s_attr[ SA_PROPER ] )
					continue;
				else if( stp1->s_checked )
					continue;
			}else
				continue;
			stp2 = stp1->s_mates[ 0 ];
			fd1 = stp1->s_index;
			ld1 = stp2->s_index;
			if( pknot[ fd1 ] == pknot[ ld1 ] )
				continue;
			stp1->s_checked = 1;
			diff = TRUE;
			for( d0 = pknot[ fd1 ], d1 = fd1; d1 < ld1; d1++ ){
				if( pknot[ d1 ] == d0 )
					pknot[ d1 ] = fd1; 
				else
					break;
			}
			if( ld1 < ld ){
				for( d0=pknot[ld1], d1=ld1; d1 < ld; d1++ ){
					if( pknot[ d1 ] == d0 )
						pknot[ d1 ] = ld1;
					else
						break;
				}
			}else{
				for( d0=pknot[ld], d1=ld+1; d1 < ld1; d1++ )
					pknot[ d1 ] = d0;
				pknot[ ld1 ] = ld1;
				ld = ld1;
			}
		}
	}

	for( d = fd; d <= ld; d++ )
		pknot[ d - fd0 ] = pknot[ d ];
	ld -= fd;

	d0 = pknot[ 0 ];
	n_pknot = 1;
	for( d = 1; d <= ld; d++ ){
		if( pknot[ d ] != d0 ){
			d0 = pknot[ d ];
			pknot[ n_pknot ] = pknot[ d ];
			n_pknot++;
		}
	}

	pknot[ n_pknot ] = UNDEF; 

	for( i = 0; i < n_pknot; i++ ){
		stps = ( STREL_T ** )malloc( n_pknot * sizeof( STREL_T * ) );
		if( stps == NULL ){
			rm_error = TRUE;
			LOG_ERROR("%s:%d can't allocate stps.", rm_wdfname, rm_lineno);
			exit(1);
		}
		for( j = 0; j < n_pknot; j++ )
			stps[ j ] = &rm_descr[ pknot[ j ] ];
		stp1 = &rm_descr[ pknot[ i ] ];
		free( stp1->s_scopes );
		stp1->s_checked = 1;
		stp1->s_scopes = stps;
		stp1->s_n_scopes = n_pknot;
		stp1->s_scope = i;
	}
}

static	int	chk_strel_parms( int n_descr, STREL_T descr[] )
{
	int	i;
	STREL_T	*stp;
	int	err;

	for( stp = descr, i = 0; i < n_descr; i++, stp++ )
		if( stp->s_mismatch == UNDEF )
			stp->s_mismatch = 0;
	for( err = FALSE, stp = descr, i = 0; i < n_descr; i++, stp++ )
		err |= chk_1_strel_parms( stp );
	if( rm_lctx != NULL ){
		if( rm_lctx->s_mismatch == UNDEF )
			rm_lctx->s_mismatch = 0;
		err |= chk_1_strel_parms( rm_lctx );
	}
	if( rm_rctx != NULL ){
		if( rm_rctx->s_mismatch == UNDEF )
			rm_rctx->s_mismatch = 0;
		err |= chk_1_strel_parms( rm_rctx );
	}
	if( !err )
		chk_strict_helices( n_descr, descr );
	return( err );
}

static	int	chk_1_strel_parms( STREL_T *stp )
{
	int	err, err1, pfrac;
	int	stype;
	STREL_T	*egroup[ 4 ];
	int	i, n_egroup;
	STREL_T	*stp1, *stpv;
	int	ival;
	float	fval;
	PAIRSET_T	*pval;
	IDENT_T	*ip;

	err = FALSE;

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
	if(	stype == SYM_CTX || stype == SYM_SS ||
		stype == SYM_P5 || stype == SYM_H5 ||
		stype == SYM_T1 || stype == SYM_Q1 )
	{
		err |= chk_len_seq( n_egroup, egroup ); 
	}

	/* check & set the pair params:					*/
	/* mispair, pairfrac, pair values, end rules and strictness	*/
	if(	stype == SYM_P5 || stype == SYM_H5 ||
		stype == SYM_T1 || stype == SYM_Q1 )
	{
		err1 = FALSE;
		for( pfrac = 0, stpv = NULL, i = 0; i < n_egroup; i++ ){
			stp1 = egroup[ i ];
			if( stp1->s_mispair != UNDEF ){
				if( stpv == NULL )
					stpv = stp1;
				else if( stpv->s_mispair != stp1->s_mispair ){
					err1 = 1;
					rm_error = TRUE;
					LOG_ERROR("%s:%d inconsistent mispair values.", rm_wdfname, stp1->s_lineno);
				}
			}else if( stp1->s_pairfrac != UNDEF ){
				pfrac = 1;
				if( stpv == NULL )
					stpv = stp1;
				else if( stpv->s_pairfrac != stp1->s_pairfrac ){
					err1 = 1;
					rm_error = TRUE;
					LOG_ERROR("%s:%d inconsistent pairfrac values.", rm_wdfname, stp1->s_lineno);
				}
			}
		}
		err |= err1;
		if( !err1 ){
			if( pfrac ){
				fval = stpv ? stpv->s_pairfrac : 1.;
				for( i = 0; i < n_egroup; i++ ){
					stp1 = egroup[ i ];
					stp1->s_pairfrac = fval;
					stp1->s_mispair = 0;
				}
			}else{
				ival = stpv ? stpv->s_mispair : 0;
				for( i = 0; i < n_egroup; i++ ){
					stp1 = egroup[ i ];
					stp1->s_mispair = ival;
					stp1->s_pairfrac = 1.0;
				}
			}
		}

		err1 = FALSE;
		for( stpv = NULL, i = 0; i < n_egroup; i++ ){
			stp1 = egroup[ i ];
			if( stp1->s_pairset != NULL ){
				if( stpv == NULL )
					stpv = stp1;
				else if( !pairop( "equal", stpv->s_pairset, stp1->s_pairset ) ){
					err1 = 1;
					rm_error = TRUE;
					LOG_ERROR("%s:%d inconsistent pairset values.", rm_wdfname, stp1->s_lineno);
				}
			}
		}
		if( stpv == NULL ){
			if( stype == SYM_P5 || stype == SYM_H5 ){
				ip = RM_find_id( "wc" );
				pval = ip->i_val.v_value.v_pval;
			}else if( stype == SYM_T1 ){
				ip = RM_find_id( "tr" );
				pval = ip->i_val.v_value.v_pval;
			}else if( stype == SYM_Q1 ){
				ip = RM_find_id( "qu" );
				pval = ip->i_val.v_value.v_pval;
			}else
				pval = NULL;
		}else
			pval = stpv->s_pairset;
		err |= err1;
		if( !err1 ){
			for( i = 0; i < n_egroup; i++ ){
				stp1 = egroup[ i ];
				if( stp1->s_pairset == NULL )
					stp1->s_pairset = pairop( "copy", pval, NULL );
			}
		}

		err1 = FALSE;
		for( stpv = NULL, i = 0; i < n_egroup; i++ ){
			stp1 = egroup[ i ];
			if( stp1->s_attr[ SA_ENDS ] != UNDEF ){
				if( stpv == NULL )
					stpv = stp1;
				else if( stpv->s_attr[ SA_ENDS ] != stp1->s_attr[ SA_ENDS ] ){
					err1 = 1;
					rm_error = TRUE;
					LOG_ERROR("%s:%d inconsistent ends values.", rm_wdfname, stp1->s_lineno);
				}
			}
		}
		if( stpv == NULL ){
			if( stype == SYM_H5 ){
				ip = RM_find_id( "wc_ends" );
				ival = ends2attr( ip->i_val.v_value.v_pval );
			}else if( stype == SYM_P5 ){
				ip = RM_find_id( "phlx_ends" );
				ival = ends2attr( ip->i_val.v_value.v_pval );
			}else if( stype == SYM_T1 ){
				ip = RM_find_id( "tr_ends" );
				ival = ends2attr( ip->i_val.v_value.v_pval );
			}else if( stype == SYM_Q1 ){
				ip = RM_find_id( "qu_ends" );
				ival = ends2attr( ip->i_val.v_value.v_pval );
			}else
				ival = FALSE;
		}else
			ival = stpv->s_attr[ SA_ENDS ];

		err |= err1;
		if( !err1 ){
			for( i = 0; i < n_egroup; i++ ){
				stp1 = egroup[ i ];
				if( stp1->s_attr[ SA_ENDS ] == UNDEF )
					stp1->s_attr[ SA_ENDS ] = ival;
			}
		}

		err1 = FALSE;
		for( stpv = NULL, i = 0; i < n_egroup; i++ ){
			stp1 = egroup[ i ];
			if( stp1->s_attr[ SA_STRICT ] != UNDEF ){
				if( stpv == NULL )
					stpv = stp1;
				else if( stpv->s_attr[ SA_STRICT ] != stp1->s_attr[ SA_STRICT ] ){
					err1 = 1;
					rm_error = TRUE;
					LOG_ERROR("%s:%d inconsistent strict values.", rm_wdfname, stp1->s_lineno);
				}
			}
		}
		if( stpv == NULL ){
			if( stype == SYM_H5 ){
				ip = RM_find_id( "wc_strict" );
				ival = strict2attr( ip->i_val.v_value.v_ival );
			}else if( stype == SYM_P5 ){
				ip = RM_find_id( "phlx_strict" );
				ival = strict2attr( ip->i_val.v_value.v_ival );
			}else if( stype == SYM_T1 ){
				ip = RM_find_id( "tr_strict" );
				ival = strict2attr( ip->i_val.v_value.v_ival );
			}else if( stype == SYM_Q1 ){
				ip = RM_find_id( "qu_strict" );
				ival = strict2attr( ip->i_val.v_value.v_ival );
			}
		}else
			ival = stpv->s_attr[ SA_ENDS ];
		err |= err1;
		if( !err1 ){
			for( i = 0; i < n_egroup; i++ ){
				stp1 = egroup[ i ];
				if( stp1->s_attr[ SA_STRICT ] == UNDEF )
					stp1->s_attr[ SA_STRICT ] = ival;
			}
		}
	}

	return( err );
}

static	int	chk_len_seq( int n_egroup, STREL_T *egroup[] )
{
	int	err, mmok;
	int	i, size;
	int	minl, maxl;
	int	x_minl, x_maxl;
	int	i_minl, i_maxl;
	int	i1_minl, i1_maxl;
	STREL_T	*stp, *stp0;
	IDENT_T	*ip;

	err = FALSE;
	stp0 = egroup[ 0 ];

	/* find explicit (len=,minlen=,maxlen=) specs	*/
	for( x_minl = x_maxl = UNDEF, i = 0; i < n_egroup; i++ ){
		stp = egroup[ i ];
		if( stp->s_minlen != UNDEF ){
			if( x_minl == UNDEF )
				x_minl = stp->s_minlen;
			else if( stp->s_minlen != x_minl ){
				err = 1;
				rm_error = TRUE;
				LOG_ERROR("%s:%d inconsistent minlen values.", rm_wdfname, stp->s_lineno);
			}
		}
		if( stp->s_maxlen != UNDEF ){
			if( x_maxl == UNDEF )
				x_maxl = stp->s_maxlen;
			else if( stp->s_maxlen != x_maxl ){
				err = 1;
				rm_error = TRUE;
				LOG_ERROR("%s:%d inconsistent maxlen values.", rm_wdfname, stp->s_lineno);
			}
		}
	}

	/* allocate reg. exp buffers		*/
	for( i = 0; i < n_egroup; i++ ){
		stp = egroup[ i ];
		if( stp->s_seq != NULL ){
			size = RE_BPC * strlen( stp->s_seq );
			stp->s_expbuf =
				( char * )malloc( size * sizeof( char ) );
			if( stp->s_expbuf == NULL ){
				rm_error = TRUE;
				LOG_ERROR("%s:%d can't allocate s_expbuf.", rm_wdfname, stp->s_lineno);
				exit(1);
			
			}
			stp->s_e_expbuf = &stp->s_expbuf[ size ];
			compile( stp->s_seq, stp->s_expbuf,
				stp->s_e_expbuf, '\0' );
		}
	}

	/* find implicit (seq=) length specs	*/
	for( i_minl = i_maxl = UNDEF, i = 0; i < n_egroup; i++ ){
		stp = egroup[ i ];
		if( stp->s_seq == NULL )
			continue;
		mm_seqlen( stp, 1, &i1_minl, &i1_maxl, &mmok );
		if( !mmok && stp->s_mismatch > 0 ){
			err = 1;
			rm_error = TRUE;
			LOG_ERROR("%s:%d mismatches not allowed in this seq.", rm_wdfname, stp->s_lineno);
		}
		if( i1_minl != UNDEF ){
			if( i_minl == UNDEF )
				i_minl = i1_minl;
			else if( i1_minl > i_minl )
				i_minl = i1_minl;
		}
		if( i1_maxl != UNDEF ){
			if( i_maxl == UNDEF )
				i_maxl = i1_maxl;
			else if( i1_maxl != i_maxl ){	/* all must agree */
				err = 1;
				rm_error = TRUE;
				LOG_ERROR("%s:%d inconsistent implied max lengths.", rm_wdfname, stp->s_lineno);
			}
		}
	}

	if( x_minl != UNDEF ){
		if( i_minl == UNDEF )
			minl = x_minl;
		else
			minl = MAX( x_minl, i_minl );
	}else if( i_minl != UNDEF ){
		minl = i_minl;
	}else if( stp0->s_type == SYM_H5 ){
		ip = RM_find_id( "wc_minlen" );
		minl = ip->i_val.v_value.v_ival;
	}else if( stp0->s_type == SYM_CTX )
		minl = 0;
	else
		minl = 1;

	if( x_maxl != UNDEF ){
		if( i_maxl == UNDEF )
			maxl = x_maxl;
		else if( x_maxl == i_maxl )
			maxl = x_maxl;
		else{
			maxl = x_maxl;
			err = 1;
			rm_error = TRUE;
			LOG_ERROR("%s:%d explicit and implicit maxlen values differ.", rm_wdfname, stp0->s_lineno);
		}
	}else if( i_maxl != UNDEF ){
		maxl = i_maxl;
	}else if( stp0->s_type == SYM_H5 ){
		ip = RM_find_id( "wc_maxlen" );
		maxl = ip->i_val.v_value.v_ival;
	}else if( stp0->s_type == SYM_CTX ){
		ip = RM_find_id( "ctx_maxlen" );
		maxl = ip->i_val.v_value.v_ival;
	}else
		maxl = UNBOUNDED;

	if( minl > maxl ){
		err = 1;
		rm_error = TRUE;
		LOG_ERROR("%s:%d minlen > maxlen.", rm_wdfname, stp0->s_lineno);
	}

	if( !err ){
		for( i = 0; i < n_egroup; i++ ){
			stp = egroup[ i ];
			stp->s_minlen = minl;
			stp->s_maxlen = maxl;
		}
	}

	return( err );
}

static	void	chk_strict_helices( int n_descr, STREL_T descr[] )
{
	int	i, sh;
	STREL_T	*stp;

	for( sh = FALSE, stp = descr, i = 0; i < n_descr; i++, stp++ ){
		if( stp->s_type == SYM_H5 ||
			stp->s_type == SYM_P5 ||
			stp->s_type == SYM_T1 ||
			stp->s_type == SYM_Q1  )
		{
			if( stp->s_attr[ SA_STRICT ] ){
				sh = TRUE;
				break;
			}
		}
	}
	rm_args->a_strict_helices = sh;
}

IDENT_T	*RM_enter_id( char name[], int type, int class, int scope, int reinit, VALUE_T *vp )
{
	IDENT_T	*ip;
	char	*np;

	ip = ( IDENT_T * )malloc( sizeof( IDENT_T ) );
	if( ip == NULL ){
		rm_error = TRUE;
		LOG_ERROR("%s:%d can't allocate ip for %s id '%s'.", rm_wdfname, rm_lineno,
			scope == S_GLOBAL ? "global" : "local", name);
		exit(1);
	}
	np = ( char * )malloc( strlen( name ) + 1 );
	if( np == NULL ){
		rm_error = TRUE;
		LOG_ERROR("%s:%d can't allocate np for name.", rm_wdfname, rm_lineno);
		exit(1);
	}
	strcpy( np, name );
	ip->i_left = NULL;
	ip->i_right = NULL;
	ip->i_name = np;
	ip->i_type = type;
	ip->i_class = class;
	ip->i_scope = scope;
	ip->i_reinit = reinit;
	ip->i_val.v_type = type;
	ip->i_val.v_value.v_pval = NULL;
	if( vp != NULL ){
		if( type == T_INT ){
			ip->i_val.v_value.v_ival = vp->v_value.v_ival;
		}else if( type == T_FLOAT ){
			ip->i_val.v_value.v_dval = vp->v_value.v_dval;
		}else if( type == T_STRING ){
			if( vp->v_value.v_pval == NULL ) 
				ip->i_val.v_value.v_pval = NULL;
			else{
				np = ( char * )
					malloc(strlen(vp->v_value.v_pval)+1);
				if( np == NULL ){
					rm_error = TRUE;
					LOG_ERROR("%s:%d can't allocate np for string val.", rm_wdfname, rm_lineno);
					exit(1);
				}
				strcpy( np, vp->v_value.v_pval );
				ip->i_val.v_value.v_pval = np;
			}
		}else if( type == T_PAIRSET ){
			ip->i_val.v_value.v_pval = pairop( "copy", 
				vp->v_value.v_pval, NULL );
		}
	}
	if( scope == S_GLOBAL ){
		rm_global_ids = enterid( rm_global_ids, ip );
		rm_n_global_ids++;
	}else{
		if( n_local_ids >= LOCAL_IDS_SIZE ){
			rm_error = TRUE;
			LOG_ERROR("%s:%d local symtab tab overflow.", rm_wdfname, rm_lineno);
			exit(1);
		}
		local_ids[ n_local_ids ] = ip;
		n_local_ids++;
	}
	return( ip );
}

IDENT_T	*RM_find_id( char name[] )
{
	int	i;
	IDENT_T	*ip;
	
	for( i = 0; i < n_local_ids; i++ ){
	 	ip = local_ids[ i ];
		if( !strcmp( name, ip->i_name ) )
			return( ip );
	}
	return( findid( rm_global_ids, name ) );
}

static	IDENT_T	*enterid( IDENT_T *root, IDENT_T *ip )
{
	int	cv;

	if( root == NULL )
		return( ip );
	else if( ( cv = strcmp( root->i_name, ip->i_name ) ) < 0 )
		root->i_right = enterid( root->i_right, ip );
	else if( cv > 0 )
		root->i_left = enterid( root->i_left, ip );
	else{
		rm_error = TRUE;
		LOG_ERROR("%s:%d attempt to redefine symbol '%s'.", rm_wdfname, rm_lineno, ip->i_name);
		exit(1);
	}
	return( root );
}

static	IDENT_T	*findid( IDENT_T *root, char *iname )
{
	int	cv;

	if( root == NULL )
		return( NULL );
	else if( ( cv = strcmp( root->i_name, iname ) ) == 0 )
		return( root );
	else if( cv < 0 )
		return( findid( root->i_right, iname ) );
	else
		return( findid( root->i_left, iname ) );
}

static	int	ends2attr( char str[] )
{
	int	slen;
	char	l_str[ 10 ], *sp, *lp;

	if( str == NULL || *str == '\0' )
		return( 0 );
	slen = strlen( str );
	if( slen != 2 ){
		rm_error = TRUE;
		LOG_ERROR("%s:%d end values are \"pp\", \"mp\", \"pm\" & \"mm\".", rm_wdfname, rm_lineno);
		return( 0 );
	}
	for( lp = l_str, sp = str; *sp; sp++ ){
		*lp++ = isupper( *sp ) ? tolower( *sp ) : *sp;
	}
	*lp = '\0';
	if( !strcmp( l_str, "pp" ) )
		return( SA_5PAIRED | SA_3PAIRED );
	else if( !strcmp( l_str, "mp" ) )
		return( SA_3PAIRED );
	else if( !strcmp( l_str, "pm" ) )
		return( SA_5PAIRED );
	else if( !strcmp( l_str, "mm" ) )
		return( 0 );
	else{
		rm_error = TRUE;
		LOG_ERROR("%s:%d end values are \"pp\", \"mp\", \"pm\" & \"mm\".", rm_wdfname, rm_lineno);
		return( 0 );
	}
}

static	int	strict2attr( int sval )
{

	if( sval == UNDEF )
		return( 0 );
	else if( sval == 0 )
		return( 0 );
	else if( sval == 1 )
		return( SA_5STRICT | SA_3STRICT );
	else if( sval == 3 )
		return( SA_3STRICT );
	else if( sval == 5 )
		return( SA_5STRICT );
	else if( sval == 35 || sval == 53 )
		return( SA_5STRICT | SA_3STRICT );
	else{
		rm_error = TRUE;
		LOG_ERROR("%s:%d strict values are 0, 1, 3, 5, 35, 53", rm_wdfname, rm_lineno);
		return( 0 );
	}
}

static	void	eval( NODE_T *expr, int d_ok )
{
	char	*sp, *l_sp, *r_sp;
	IDENT_T	*ip;
	int	l_type, r_type;
	PAIRSET_T	*n_ps, *l_ps, *r_ps;
	POS_T	*l_pos, *r_pos, *n_pos;

	if( expr ){
		eval( expr->n_left, d_ok );
		eval( expr->n_right, d_ok );
		switch( expr->n_sym ){

		case SYM_INT :
			valstk[ n_valstk ].v_type = T_INT;
			valstk[ n_valstk ].v_value.v_ival =
				expr->n_val.v_value.v_ival;
			n_valstk++;
			break;

		case SYM_FLOAT :
			valstk[ n_valstk ].v_type = T_FLOAT;
			valstk[ n_valstk ].v_value.v_dval =
				expr->n_val.v_value.v_dval;
			n_valstk++;
			break;

		case SYM_STRING :
			sp = ( char * )
				malloc(strlen( expr->n_val.v_value.v_pval )+1);
			if( sp == NULL ){
				rm_error = TRUE;
				LOG_ERROR("%s:%d can't allocate sp for string.", rm_wdfname, rm_lineno);
				exit(1);
			}
			strcpy( sp, expr->n_val.v_value.v_pval );
			valstk[ n_valstk ].v_type = T_STRING;
			valstk[ n_valstk ].v_value.v_pval = sp;
			n_valstk++;
			break;

		case SYM_PAIRSET :
			valstk[ n_valstk ].v_type = T_PAIRSET;
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
			ip = RM_find_id( expr->n_val.v_value.v_pval );
			if( ip == NULL ){
				if( d_ok ){
					ip=RM_enter_id(
						expr->n_val.v_value.v_pval,
						T_UNDEF, C_VAR, S_GLOBAL, FALSE,
						NULL);
				}else{
					rm_error = TRUE;
					LOG_ERROR("%s:%d unknown id '%s'.", rm_wdfname, rm_lineno, (char *)expr->n_val.v_value.v_pval);
					exit(1);
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

			switch( T_IJ( l_type, r_type ) ){ 
			case T_IJ( T_INT, T_INT ) :
				valstk[ n_valstk - 2 ].v_value.v_ival +=
					valstk[ n_valstk - 1 ].v_value.v_ival;
				break;
			case T_IJ( T_INT, T_FLOAT ) :
				valstk[ n_valstk - 2 ].v_value.v_ival +=
					valstk[ n_valstk - 1 ].v_value.v_dval;
				break;
			case T_IJ( T_FLOAT, T_INT ) :
				valstk[ n_valstk - 2 ].v_value.v_dval +=
					valstk[ n_valstk - 1 ].v_value.v_ival;
				break;
			case T_IJ( T_FLOAT, T_FLOAT ) :
				valstk[ n_valstk - 2 ].v_value.v_dval +=
					valstk[ n_valstk - 1 ].v_value.v_dval;
				break;
			case T_IJ( T_STRING, T_STRING ) :
				l_sp = valstk[ n_valstk - 2 ].v_value.v_pval;
				r_sp = valstk[ n_valstk - 1 ].v_value.v_pval;
				sp = ( char * )malloc( strlen( l_sp ) +
					strlen( r_sp ) + 1 );
				if( sp == NULL ){
					rm_error = TRUE;
					LOG_ERROR("%s:%d can't allocate sp for str +.", rm_wdfname, rm_lineno);
					exit(1);
				}
				strcpy( sp, l_sp );
				strcat( sp, r_sp );
				valstk[ n_valstk - 2 ].v_value.v_pval = sp;
				break;
			case T_IJ( T_PAIRSET, T_PAIRSET ) :
				l_ps = valstk[ n_valstk - 2 ].v_value.v_pval;
				r_ps = valstk[ n_valstk - 1 ].v_value.v_pval;
				n_ps = pairop( "add", l_ps, r_ps );
				valstk[ n_valstk - 2 ].v_value.v_pval = n_ps;
				break;
			default :
				rm_error = TRUE;
				LOG_ERROR("%s:%d type mismatch '+'.", rm_wdfname, rm_lineno);
				exit(1);
				break;
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

			switch( T_IJ( l_type, r_type ) ){
			case T_IJ( T_INT, T_INT ) :
				valstk[ n_valstk - 2 ].v_value.v_ival -=
					valstk[ n_valstk - 1 ].v_value.v_ival;
				break;
			case T_IJ( T_INT, T_FLOAT ) :
				valstk[ n_valstk - 2 ].v_value.v_ival -=
					valstk[ n_valstk - 1 ].v_value.v_dval;
				break;
			case T_IJ( T_FLOAT, T_INT ) :
				valstk[ n_valstk - 2 ].v_value.v_dval -=
					valstk[ n_valstk - 1 ].v_value.v_ival;
				break;
			case T_IJ( T_FLOAT, T_FLOAT ) :
				valstk[ n_valstk - 2 ].v_value.v_dval -=
					valstk[ n_valstk - 1 ].v_value.v_dval;
				break;
			case T_IJ( T_PAIRSET, T_PAIRSET ) :
				l_ps = valstk[ n_valstk - 2 ].v_value.v_pval;
				r_ps = valstk[ n_valstk - 1 ].v_value.v_pval;
				n_ps = pairop( "sub", l_ps, r_ps );
				valstk[ n_valstk - 2 ].v_value.v_pval = n_ps;
				break;
			case T_IJ( T_POS, T_INT ) :
				l_pos = valstk[ n_valstk - 2 ].v_value.v_pval;
				posop( "cvt", &valstk[ n_valstk - 1 ], NULL );
				r_pos = valstk[ n_valstk - 1 ].v_value.v_pval;
				n_pos = posop( "sub", l_pos, r_pos );
				valstk[ n_valstk - 2 ].v_value.v_pval = n_pos;
				break;
			default :
				rm_error = TRUE;
				LOG_ERROR("%s:%d type mismatch '-'.", rm_wdfname, rm_lineno);
				exit(1);
				break;
			}
			n_valstk--;
			break;

		case SYM_NEGATE :
			r_type = valstk[ n_valstk - 1 ].v_type;
			if( r_type == T_IDENT )
				r_type = loadidval( &valstk[ n_valstk - 2 ] );
			switch( r_type ){
			case T_INT :
				valstk[ n_valstk - 1 ].v_value.v_ival =
					-valstk[ n_valstk - 1 ].v_value.v_ival;
				break;
			case T_FLOAT :
				valstk[ n_valstk - 1 ].v_value.v_dval =
					-valstk[ n_valstk - 1 ].v_value.v_dval;
				break;
			default :
				rm_error = TRUE;
				LOG_ERROR("%s:%d type mismatch '-'.", rm_wdfname, rm_lineno);
				exit(1);
				break;
			}
			break;

		case SYM_ASSIGN :
			ip = valstk[ n_valstk - 2 ].v_value.v_pval;
			l_type = ip->i_type;
			r_type = valstk[ n_valstk - 1 ].v_type;
			if( r_type == T_IDENT )
				r_type = loadidval( &valstk[ n_valstk - 1 ] );
			if( l_type == T_UNDEF ){
				l_type = r_type; 
				ip->i_type = l_type;
			}
			switch(  T_IJ( l_type, r_type ) ){
			case T_IJ( T_INT, T_INT ) :
				break;
			case T_IJ( T_INT, T_FLOAT ) :
				/* promote _variable_ to float: */
				valstk[n_valstk-1].v_type = T_INT;
				valstk[n_valstk-1].v_value.v_ival =
					valstk[n_valstk-1].v_value.v_dval;
				break;
			case T_IJ( T_FLOAT, T_INT ) :
				/* promote _variable_ to float: */
				valstk[n_valstk-1].v_type = T_FLOAT;
				valstk[n_valstk-1].v_value.v_dval =
					valstk[n_valstk-1].v_value.v_ival;
				break;
			case T_IJ( T_FLOAT, T_FLOAT ) :
				break;
			case T_IJ( T_STRING, T_STRING ) :
				break;
			case T_IJ( T_PAIRSET, T_PAIRSET ) :
				break;
			case T_IJ( T_POS, T_INT ) :
				posop( "cvt", &valstk[ n_valstk - 1 ], NULL );
				break;
			case T_IJ( T_POS, T_POS ) :
				break;
			default :
				rm_error = TRUE;
				LOG_ERROR("%s:%d type mismatch '='.", rm_wdfname, rm_lineno);
				exit(1);
				break;
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

			switch( T_IJ( l_type, r_type ) ){
			case T_IJ( T_INT, T_INT ) :
				valstk[ n_valstk - 2 ].v_value.v_ival +=
					valstk[ n_valstk - 1 ].v_value.v_ival;
				break;
			case T_IJ( T_INT, T_FLOAT ) :
				valstk[ n_valstk - 2 ].v_value.v_ival +=
					valstk[ n_valstk - 1 ].v_value.v_dval;
				break;
			case T_IJ( T_FLOAT, T_INT ) :
				valstk[ n_valstk - 2 ].v_value.v_dval +=
					valstk[ n_valstk - 1 ].v_value.v_ival;
				break;
			case T_IJ( T_FLOAT, T_FLOAT ) :
				valstk[ n_valstk - 2 ].v_value.v_dval +=
					valstk[ n_valstk - 1 ].v_value.v_dval;
				break;
			case T_IJ( T_STRING, T_STRING ) :
				l_sp = valstk[ n_valstk - 2 ].v_value.v_pval;
				r_sp = valstk[ n_valstk - 1 ].v_value.v_pval;
				sp = ( char * )malloc( strlen( l_sp ) +
					strlen( r_sp ) + 1 );
				if( sp == NULL ){
					rm_error = TRUE;
					LOG_ERROR("%s:%d can't allocate sp for str '+='.", rm_wdfname, rm_lineno);
					exit(1);
				}
				strcpy( sp, l_sp );
				strcat( sp, r_sp );
				valstk[ n_valstk - 2 ].v_value.v_pval = sp;
				break;
			case T_IJ( T_PAIRSET, T_PAIRSET ) :
				l_ps = valstk[ n_valstk - 2 ].v_value.v_pval;
				r_ps = valstk[ n_valstk - 1 ].v_value.v_pval;
				n_ps = pairop( "add", l_ps, r_ps );
				valstk[ n_valstk - 2 ].v_value.v_pval = n_ps;
				break;
			default :
				rm_error = TRUE;
				LOG_ERROR("%s:%d type mismatch '+='.", rm_wdfname, rm_lineno);
				exit(1);
				break;
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

			switch( T_IJ( l_type, r_type ) ){
			case T_IJ( T_INT, T_INT ) :
				valstk[ n_valstk - 2 ].v_value.v_ival -=
					valstk[ n_valstk - 1 ].v_value.v_ival;
				break;
			case T_IJ( T_INT, T_FLOAT ) :
				valstk[ n_valstk - 2 ].v_value.v_ival -=
					valstk[ n_valstk - 1 ].v_value.v_dval;
				break;
			case T_IJ( T_FLOAT, T_INT ) :
				valstk[ n_valstk - 2 ].v_value.v_dval -=
					valstk[ n_valstk - 1 ].v_value.v_ival;
				break;
			case T_IJ( T_FLOAT, T_FLOAT ) :
				valstk[ n_valstk - 2 ].v_value.v_dval -=
					valstk[ n_valstk - 1 ].v_value.v_dval;
				break;
			case T_IJ( T_PAIRSET, T_PAIRSET ) :
				l_ps = valstk[ n_valstk - 2 ].v_value.v_pval;
				r_ps = valstk[ n_valstk - 1 ].v_value.v_pval;
				n_ps = pairop( "sub", l_ps, r_ps );
				valstk[ n_valstk - 2 ].v_value.v_pval = n_ps;
				break;
			default :
				rm_error = TRUE;
				LOG_ERROR("%s:%d type mimatch '-='.", rm_wdfname, rm_lineno);
				exit(1);
				break;
			}
			storeexprval( ip, &valstk[ n_valstk - 2 ] );
			n_valstk -= 2;
			break;

		default :
			rm_error = TRUE;
			LOG_ERROR("%s:%d operator %d not implemented.", rm_wdfname, rm_lineno, expr->n_sym);
			exit(1);
			break;
		}
	}
}

static	int	loadidval( VALUE_T *vp )
{
	int	type;
	IDENT_T	*ip;
	char	*sp;
	PAIRSET_T	*ps = NULL;

	ip = vp->v_value.v_pval;
	type = ip->i_type;
	if( type == T_INT ){
		if( ip->i_val.v_value.v_ival == UNDEF ){
			rm_error = TRUE;
			LOG_ERROR("%s:%d id '%s' has int value UNDER.", rm_wdfname, rm_lineno, ip->i_name);
			exit(1);
		}
		vp->v_type = T_INT;
		vp->v_value.v_ival = ip->i_val.v_value.v_ival;
	}else if( type == T_FLOAT ){
		if( ip->i_val.v_value.v_ival == UNDEF ){
			rm_error = TRUE;
			LOG_ERROR("%s:%d id '%s' has int value UNDEF.", rm_wdfname, rm_lineno, ip->i_name);
			exit(1);
		}
		vp->v_type = T_FLOAT;
		vp->v_value.v_dval = ip->i_val.v_value.v_dval;
	}else if( type == T_STRING ){
		if( ip->i_val.v_value.v_pval == NULL ){
			rm_error = TRUE;
			LOG_ERROR("%s:%d id '%s' has string value NULL.", rm_wdfname, rm_lineno, ip->i_name);
			exit(1);
		}
		sp = ( char * )malloc( strlen( ip->i_val.v_value.v_pval ) + 1 );
		if( sp == NULL ){
			rm_error = TRUE;
			LOG_ERROR("%s:%d can't allocate sp for '%s'.", rm_wdfname, rm_lineno, ip->i_name);
			exit(1);
		}
		vp->v_type = T_STRING;
		strcpy( sp, ip->i_val.v_value.v_pval );
		vp->v_value.v_pval = sp;
	}else if( type == T_PAIRSET ){
		if( ip->i_val.v_value.v_pval == NULL ){
			if( open_pairset != NULL )
				ps = open_pairset;
			else{
				rm_error = TRUE;
				LOG_ERROR("%s:%d id '%s' has pair value NULL.", rm_wdfname, rm_lineno, ip->i_name);
				exit(1);
			}
		}else
			ps = ip->i_val.v_value.v_pval;
		vp->v_type = T_PAIRSET;
		vp->v_value.v_pval = pairop( "copy", ps, NULL );
	}
	return( type );
}

static	void	storeexprval( IDENT_T *ip, VALUE_T *vp )
{
	int	type;
	char	*sp;


	type = vp->v_type;
	if( type == T_INT ){
		ip->i_type = T_INT;
		ip->i_val.v_type = T_INT;
		ip->i_val.v_value.v_ival = vp->v_value.v_ival;
	}else if( type == T_FLOAT ){
		ip->i_type = T_FLOAT;
		ip->i_val.v_type = T_FLOAT;
		ip->i_val.v_value.v_dval = vp->v_value.v_dval;
	}else if( type == T_STRING ){
		ip->i_type = T_STRING;
		sp = ( char * )malloc( strlen( vp->v_value.v_pval ) + 1 );
		if( sp == NULL ){
			rm_error = TRUE;
			LOG_ERROR("%s:%d can't allocate sp.", rm_wdfname, rm_lineno);
			exit(1);
		}
		strcpy( sp, vp->v_value.v_pval ); 
		ip->i_val.v_type = T_STRING;
		ip->i_val.v_value.v_pval = sp;
	}else if( type == T_PAIRSET ){
		ip->i_type = T_PAIRSET;
		ip->i_val.v_type = T_PAIRSET;
		ip->i_val.v_value.v_pval = vp->v_value.v_pval;
	}else if( type == T_POS ){
		ip->i_type = T_POS;
		ip->i_val.v_type = T_POS;
		ip->i_val.v_value.v_pval = vp->v_value.v_pval;
	}
}

static	PAIRSET_T	*pairop( char op[], PAIRSET_T *ps1, PAIRSET_T *ps2 )
{
	int	i, j, c, b, nb, sz, diff, fnd;
	PAIRSET_T	*n_ps;
	PAIR_T	*n_pp, *pp, *ppi, *ppj;

	if( !strcmp( op, "check" ) ){
		if( ps1 == NULL ){
			rm_error = TRUE;
			LOG_ERROR("%s:%d check: ps1 == NULL.", rm_wdfname, rm_lineno);
			exit(1);
		}
		pp = ps1->ps_pairs;
		for( nb = UNDEF, i = 0; i < ps1->ps_n_pairs; i++, pp++ ){
			b = pp->p_n_bases;
			if( nb == UNDEF )
				nb = b;
			else if( b != nb ){
				rm_error = TRUE;
				LOG_ERROR("%s:%d check: pairset contains elements with %d and %d bases.",
					rm_wdfname, rm_lineno, nb, b);
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
				for(diff=FALSE, b=0; b < ppi->p_n_bases; b++ ){
					if( ppi->p_bases[b]!=ppj->p_bases[b]){
						diff = TRUE;
						break;
					}
				}
				if( !diff ){
					ppj->p_n_bases = 0;
					rm_error = TRUE;
					LOG_ERROR("%s:%d check: pairset contains duplicate pair-strings.", rm_wdfname, rm_lineno);
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
		pp = ps1->ps_pairs;
		if( pp->p_n_bases == 2 )
			ps1->ps_mat[ 0 ] = mk_bmatp( ps1 );
		else{
			ps1->ps_mat[ 1 ] = mk_bmatp( ps1 );
			ps1->ps_mat[ 0 ] = mk_rbmatp( ps1 );
		}
		return( ps1 );
	}else if( !strcmp( op, "copy" ) ){
		if( ps1 == NULL )
			return( NULL );
		n_ps = ( PAIRSET_T * )malloc( sizeof( PAIRSET_T ) );
		if( n_ps == NULL ){
			rm_error = TRUE;
			LOG_ERROR("%s:%d copy: can't allocate n_ps.", rm_wdfname, rm_lineno);
			exit(1);
		}
		n_pp = ( PAIR_T * )malloc( ps1->ps_n_pairs*sizeof( PAIR_T ) );
		if( n_pp == NULL ){
			rm_error = TRUE;
			LOG_ERROR("%s:%d copy: can't allocate n_pp.", rm_wdfname, rm_lineno);
			exit(1);
		}
		n_ps->ps_n_pairs = ps1->ps_n_pairs;
		n_ps->ps_pairs = n_pp;
		for( i = 0; i < n_ps->ps_n_pairs; i++ )
			n_ps->ps_pairs[ i ] = ps1->ps_pairs[ i ];
		pp = n_ps->ps_pairs;
		if( pp->p_n_bases == 2 )
			n_ps->ps_mat[ 0 ] = mk_bmatp( n_ps );
		else{
			n_ps->ps_mat[ 1 ] = mk_bmatp( n_ps );
			n_ps->ps_mat[ 0 ] = mk_rbmatp( n_ps );
		}
		return( n_ps );
	}else if( !strcmp( op, "add" ) ){
		ppi = ps1->ps_pairs;
		ppj = ps2->ps_pairs;
		if( ppi->p_n_bases != ppj->p_n_bases ){
			rm_error = TRUE;
			LOG_ERROR("%s:%d add: pairsets have %d and %d elements.",
				rm_wdfname, rm_lineno, ppi->p_n_bases, ppj->p_n_bases);
			exit(1);
		}
		sz = ps1->ps_n_pairs + ps2->ps_n_pairs;
		n_ps = ( PAIRSET_T * )malloc( sizeof( PAIRSET_T ) );
		if( n_ps == NULL ){
			rm_error = TRUE;
			LOG_ERROR("%s:%d add: can't allocate n_ps.", rm_wdfname, rm_lineno);
			exit(1);
		}
		n_pp = ( PAIR_T * )malloc( sz * sizeof( PAIR_T ) );
		if( n_pp == NULL ){
			rm_error = TRUE;
			LOG_ERROR("%s:%d add: can't allocate n_pp.", rm_wdfname, rm_lineno);
			exit(1);
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
		pp = n_ps->ps_pairs;
		if( pp->p_n_bases == 2 )
			n_ps->ps_mat[ 0 ] = mk_bmatp( n_ps );
		else{
			n_ps->ps_mat[ 1 ] = mk_bmatp( n_ps );
			n_ps->ps_mat[ 0 ] = mk_rbmatp( n_ps );
		}
		return( n_ps );
	}else if( !strcmp( op, "sub" ) ){
		ppi = ps1->ps_pairs;
		ppj = ps2->ps_pairs;
		if( ppi->p_n_bases != ppj->p_n_bases ){
			rm_error = TRUE;
			LOG_ERROR("%s:%d sub: pairsets have %d and %d elements.",
				rm_wdfname, rm_lineno, ppi->p_n_bases, ppj->p_n_bases);
			exit(1);
		}
		sz = ps1->ps_n_pairs;
		n_ps = ( PAIRSET_T * )malloc( sizeof( PAIRSET_T ) );
		if( n_ps == NULL ){
			rm_error = TRUE;
			LOG_ERROR("%s:%d add: can't allocate n_ps.", rm_wdfname, rm_lineno);
			exit(1);
		}
		n_pp = ( PAIR_T * )malloc( sz * sizeof( PAIR_T ) );
		if( n_pp == NULL ){
			rm_error = TRUE;
			LOG_ERROR("%s:%d add: can't allocate n_pp.", rm_wdfname, rm_lineno);
			exit(1);
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
		pp = n_ps->ps_pairs;
		if( pp->p_n_bases == 2 )
			n_ps->ps_mat[ 0 ] = mk_bmatp( n_ps );
		else{
			n_ps->ps_mat[ 1 ] = mk_bmatp( n_ps );
			n_ps->ps_mat[ 0 ] = mk_rbmatp( n_ps );
		}
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
				for( fnd=TRUE, b = 0; b < ppi->p_n_bases; b++ ){
					if( ppi->p_bases[b]!=ppj->p_bases[b] ){
						fnd = FALSE;
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
		rm_error = TRUE;
		LOG_ERROR("%s:%d unknown op '%s'.", rm_wdfname, rm_lineno, op);
		exit(1);
		return( NULL );
	}
}

static	void*	mk_bmatp( PAIRSET_T *ps )
{
	PAIR_T	*pp;
	int	i, nb;
	int	bi1, bi2, bi3, bi4;
	void	*bmatp = NULL;
	BP_MAT_T	*bpmatp;
	BT_MAT_T	*btmatp;
	BQ_MAT_T	*bqmatp;

	pp = &ps->ps_pairs[ 0 ];
	nb = pp->p_n_bases;
	if( nb == 2 ){
		bmatp = bpmatp = ( BP_MAT_T * )malloc( sizeof( BP_MAT_T ) );
		if( bpmatp == NULL ){
			rm_error = TRUE;
			LOG_ERROR("%s:%d can't allocate bpmatp", rm_wdfname, rm_lineno);
			exit(1);
		}
		memset( bmatp, 0, sizeof( BP_MAT_T ) );
		for( i = 0; i < ps->ps_n_pairs; i++ ){
			pp = &ps->ps_pairs[ i ];
			bi1 = rm_b2bc[ (int)pp->p_bases[ 0 ] ];
			bi2 = rm_b2bc[ (int)pp->p_bases[ 1 ] ];
			(*bpmatp)[bi1][bi2] = 1;
		}
	}else if( nb == 3 ){
		bmatp = btmatp = ( BT_MAT_T * )malloc( sizeof( BT_MAT_T ) );
		if( btmatp == NULL ){
			rm_error = TRUE;
			LOG_ERROR("%s:%d can't allocate btmatp.", rm_wdfname, rm_lineno);
			exit(1);
		}
		memset( bmatp, 0, sizeof( BT_MAT_T ) );
		for( i = 0; i < ps->ps_n_pairs; i++ ){
			pp = &ps->ps_pairs[ i ];
			bi1 = rm_b2bc[ (int)pp->p_bases[ 0 ] ];
			bi2 = rm_b2bc[ (int)pp->p_bases[ 1 ] ];
			bi3 = rm_b2bc[ (int)pp->p_bases[ 2 ] ];
			(*btmatp)[bi1][bi2][bi3] = 1;
		}
	}else if( nb == 4 ){
		bmatp = bqmatp = ( BQ_MAT_T * )malloc( sizeof( BQ_MAT_T ) );
		if( bqmatp == NULL ){
			rm_error = TRUE;
			LOG_ERROR("%s:%d can't allocate bpmatp.", rm_wdfname, rm_lineno);
			exit(1);
		}
		memset( bmatp, 0, sizeof( BQ_MAT_T ) );
		for( i = 0; i < ps->ps_n_pairs; i++ ){
			pp = &ps->ps_pairs[ i ];
			bi1 = rm_b2bc[ (int)pp->p_bases[ 0 ] ];
			bi2 = rm_b2bc[ (int)pp->p_bases[ 1 ] ];
			bi3 = rm_b2bc[ (int)pp->p_bases[ 2 ] ];
			bi4 = rm_b2bc[ (int)pp->p_bases[ 3 ] ];
			(*bqmatp)[bi1][bi2][bi3][bi4] = 1;
		}
	}
	return( bmatp );
}

static	void*	mk_rbmatp( PAIRSET_T *ps )
{
	PAIR_T	*pp;
	int	nb;
	int	bi1, bi2, bi3, bi4;
	void	*bmatp;
	BP_MAT_T	*bpmatp;
	BT_MAT_T	*btmatp;
	BQ_MAT_T	*bqmatp;

	pp = &ps->ps_pairs[ 0 ];
	nb = pp->p_n_bases;
	bmatp = bpmatp = ( BP_MAT_T * )malloc( sizeof( BP_MAT_T ) );
	if( bpmatp == NULL ){
		rm_error = TRUE;
		LOG_ERROR("%s:%d can't allocate bpmatp.", rm_wdfname, rm_lineno);
		exit(1);
	}
	memset( bmatp, 0, sizeof( BP_MAT_T ) );

	if( nb == 3 ){
		btmatp = ( BT_MAT_T * )ps->ps_mat[ 1 ];
		for( bi1 = 0; bi1 < N_BCODES; bi1++ ){
			for( bi2 = 0; bi2 < N_BCODES; bi2++ ){
				for( bi3 = 0; bi3 < N_BCODES; bi3++ ){
					if( (*btmatp)[bi1][bi2][bi3] )
						(*bpmatp)[bi1][bi3] = 1;
				}
			}
		}
	}else if( nb == 4 ){
		bqmatp = ( BQ_MAT_T * )ps->ps_mat[ 1 ];
		for( bi1 = 0; bi1 < N_BCODES; bi1++ ){
			for( bi2 = 0; bi2 < N_BCODES; bi2++ ){
				for( bi3 = 0; bi3 < N_BCODES; bi3++ ){
					for( bi4 = 0; bi4 < N_BCODES; bi4++ ){
					    if((*bqmatp)[bi1][bi2][bi3][bi4])
						(*bpmatp)[bi1][bi4] = 1;
					}
				}
			}
		}
	}
	return( bmatp );
}

static	POS_T	*posop( char op[], void *ptr, POS_T *r_pos )
{
	VALUE_T	*vp;
	POS_T	*n_pos, *l_pos;

	if( !strcmp( op, "cvt" ) ){
		vp = ( VALUE_T * )ptr;
		if( vp->v_value.v_ival < 0 ){
			rm_error = TRUE;
			LOG_ERROR("%s:%d cvt: only ints > 0 can be converted to positions.", rm_wdfname, rm_lineno);
			exit(1);
		}
		n_pos = ( POS_T * )malloc( sizeof( POS_T ) );
		if( n_pos == NULL ){
			rm_error = TRUE;
			LOG_ERROR("%s:%d cvt: can't allocate n_pos.", rm_wdfname, rm_lineno);
			exit(1);
		}
		n_pos->p_type = SYM_DOLLAR;
		n_pos->p_lineno = rm_lineno;
		n_pos->p_tag = NULL;
		n_pos->p_addr.a_l2r = 1;
		n_pos->p_addr.a_offset = vp->v_value.v_ival;
		vp->v_type = T_POS;
		vp->v_value.v_pval = n_pos;
		return( n_pos );
	}else if( !strcmp( op, "sub" ) ){
		l_pos = ( POS_T * )ptr;
		if( l_pos->p_addr.a_l2r || !r_pos->p_addr.a_l2r ){
			rm_error = TRUE;
			LOG_ERROR("%s:%d sub: expr must have the form '$ - expr'; expr is int valued > 0.",
				rm_wdfname, rm_lineno);
			exit(1);
		}
		n_pos = ( POS_T * )malloc( sizeof( POS_T ) );
		if( n_pos == NULL ){
			rm_error = TRUE;
			LOG_ERROR("%s:%d sub: can't allocate n_pos.", rm_wdfname, rm_lineno);
			exit(1);
		}
		n_pos->p_type = SYM_DOLLAR;
		n_pos->p_lineno = rm_lineno;
		n_pos->p_tag = NULL;
		n_pos->p_addr.a_l2r = FALSE;
		n_pos->p_addr.a_offset =
			l_pos->p_addr.a_offset + r_pos->p_addr.a_offset;
		return( n_pos );
	}else{
		rm_error = TRUE;
		LOG_ERROR("%s:%d unknown op '%s'.", rm_wdfname, rm_lineno, op);
		exit(1);
		return( NULL );
	}
}

char	*RM_str2seq( char str[] )
{
	char	*s1, *s2, *s3, *sp;
	int	iupac, c;
	IDENT_T	*ip;
	char	seq[ 1024 ];

	if( str == NULL || *str == '\0' ){
		sp = ( char * )malloc( 1 * sizeof( char ) );
		if( sp == NULL ){
			rm_error = TRUE;
			LOG_ERROR("%s:%d can't allocate sp.", rm_wdfname, rm_lineno);
			exit(1);
		}
		*sp = '\0';
		return( sp );
	}
	ip = RM_find_id( "iupac" );
	if( ip )
		iupac = ip->i_val.v_value.v_ival;
	else
		iupac = 0;
	for( s1 = str, s2 = seq; *s1; s1++ ){
		c = isupper( *s1 ) ? tolower( *s1 ) : *s1;
		if( c == 'u' )
			c = 't';
		if( iupac ){
			if((s3 = rm_iupac[ c ])){
				strcpy( s2, s3 );
				s2 += strlen( s3 );
			}else
				*s2++ = c;
		}else
			*s2++ = c;
	}
	*s2 = '\0';
	sp = ( char * )malloc( strlen( seq ) + 1 );
	if( sp == NULL ){
		rm_error = TRUE;
		LOG_ERROR("%s:%d can't allocate sp.", rm_wdfname, rm_lineno);
		exit(1);
	}
	strcpy( sp, seq );
	return( sp );
}

void	POS_open( int ptype )
{
	VALUE_T	val;

	n_valstk = 0;
	if( ptype == SYM_SE ){
		rm_error = TRUE;
		LOG_ERROR("%s:%d site tpe 'se' allowed only in score section.", rm_wdfname, rm_lineno);
		exit(1);
	}
	if( rm_n_pos == rm_s_pos ){
		rm_error = TRUE;
		LOG_ERROR("%s:%d pos array size(%d) esceeded.", rm_wdfname, rm_lineno, rm_s_pos);
		exit(1);
	}
	posp = &rm_pos[ rm_n_pos ];
	rm_n_pos++;
	posp->p_type = ptype;
	posp->p_lineno = rm_lineno;
	posp->p_tag = NULL;
	posp->p_descr = NULL;
	posp->p_addr.a_l2r = 1;	/* TRUE = 1..$; FALSE = $..1	*/
	posp->p_addr.a_offset = 0;

	n_local_ids = 0;
	val.v_type = T_STRING;
	val.v_value.v_pval = NULL;
	RM_enter_id( "tag", T_STRING, C_VAR, S_SITE, FALSE, &val );

	val.v_type = T_POS;
	val.v_value.v_pval = NULL;
	RM_enter_id( "pos", T_POS, C_VAR, S_SITE, FALSE, &val );
}

void	POS_addval( NODE_T *expr )
{

}

void	POS_close( void )
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
			posp->p_addr.a_l2r = i_pos->p_addr.a_l2r;
			posp->p_addr.a_offset = i_pos->p_addr.a_offset;
		}
	}
	n_local_ids = 0;
}

void	SI_close( NODE_T *expr )
{
	int	i;
	POS_T	*posp;
	PAIRSET_T	*ps;
	SITE_T	*sip, *sip1;

	posp = ( POS_T * )malloc( rm_n_pos * sizeof( POS_T ) );
	if( posp == NULL ){
		rm_error = TRUE;
		LOG_ERROR("%s:%d can't allocate posp.", rm_wdfname, rm_lineno);
		exit(1);
	}
	for( i = 0; i < rm_n_pos; i++ )
		posp[ i ] = rm_pos[ i ];
	sip = ( SITE_T * )malloc( sizeof( SITE_T ) );
	if( sip == NULL ){
		rm_error = TRUE;
		LOG_ERROR("%s:%d can't allocate sip.", rm_wdfname, rm_lineno);
		exit(1);
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

static	int	chk_site( SITE_T *sip )
{
	int	err;
	int	i, j;
	POS_T	*posp;
	char	*sp;
	STREL_T	*stp;

	err = FALSE;
	if( sip->s_n_pos != sip->s_pairset->ps_pairs[ 0 ].p_n_bases ){
		err = TRUE;
		rm_error = TRUE;
		LOG_ERROR("%s:%d Number of positions in site must agree with pairset.", rm_wdfname, sip->s_pos[0].p_lineno);
	}
	for( posp = sip->s_pos, i = 0; i < sip->s_n_pos; i++, posp++ ){
		if( ( sp = posp->p_tag ) == NULL ){
			err = 1;
			rm_error = TRUE;
			LOG_ERROR("%s:%d all positions must be tagged.", rm_wdfname, posp->p_lineno);
		}else{
			stp = rm_descr;
			for( j = 0; j < rm_n_descr; j++, stp++ ){
				if( stp->s_tag == NULL )
					continue;
				if( stp->s_type != posp->p_type )
					continue;
				if( !strcmp( sp, stp->s_tag ) ){
					posp->p_descr = stp;
					break;
				}
			}
			if( posp->p_descr == NULL ){
				err = 1;
				rm_error = TRUE;
				LOG_ERROR("%s:%d position with undefined tag '%s'.", rm_wdfname, posp->p_lineno, sp);
			}
		}
	}
	for( posp = sip->s_pos, i = 0; i < sip->s_n_pos; i++, posp++ ){
		if( posp->p_descr == NULL )
			continue;
		stp = posp->p_descr;
		if( posp->p_addr.a_l2r ){
			if( posp->p_addr.a_offset > stp->s_minlen ){
				err = 1;
				rm_error = TRUE;
				LOG_ERROR("%s:%d position offset > strel minlen.", rm_wdfname, posp->p_lineno);
			}
		}else if( posp->p_addr.a_offset + 1 > stp->s_minlen ){
			err = 1;
			rm_error = TRUE;
			LOG_ERROR("%s:%d position offset > strel minlen.", rm_wdfname, posp->p_lineno);
		}
	}

	return( err );
}

static	STREL_T	*set_scopes( int fd, int ld, STREL_T descr[] )
{
	int	d, nd, s;
	int	fd1, ld1;
	STREL_T	*stp, *stp1, *stp2;

	if( fd > ld )
		return( NULL );

	for( d = fd; d <= ld; d = nd ){
		stp = &descr[ d ];
		stp->s_outer = scope_stk[ t_scope_stk ];
		if( stp->s_n_scopes == 0 ){
			nd = d + 1;
			if( nd <= ld ){
				stp->s_next = &descr[ nd ];
				stp->s_next->s_prev = stp;
			}
			continue;
		}
		t_scope_stk++;
		scope_stk[ t_scope_stk ] = stp;
		for( s = 0; s < stp->s_n_scopes - 1; s++ ){
			stp1 = stp->s_scopes[ s ];
			stp2 = stp->s_scopes[ s + 1 ];
			fd1 = stp1->s_index + 1;
			ld1 = stp2->s_index - 1;
			t_scope_stk++;
			scope_stk[ t_scope_stk ] = stp1;
			stp1->s_inner = set_scopes( fd1, ld1, descr );
			t_scope_stk--;
			stp2->s_outer = scope_stk[ t_scope_stk ];
		}
		stp2 = stp->s_scopes[ stp->s_n_scopes - 1 ];
		t_scope_stk--;
		nd = stp2->s_index + 1;
		if( nd <= ld ){
			stp->s_next = &descr[ nd ];
			stp->s_next->s_prev = stp;
		}
	}
	return( &descr[ fd ] );
}

static	void	find_gi_len( int fd, STREL_T descr[], int *tminlen, int *tmaxlen )
{
	int	d, d1, nd;
	int	gminlen, gmaxlen;
	int	minlen3;
	int	maxlen3;
	STREL_T	*stp, *stp1, *stp2, *stp3;

	*tminlen = 0;
	*tmaxlen = 0;
	for( d = fd; ; d = nd ){
		stp = &descr[ d ];
		gminlen = stp->s_minlen;
		gmaxlen = stp->s_maxlen;
		for( d1 = 0; d1 < stp->s_n_scopes - 1; d1++ ){
			stp1 = stp->s_scopes[ d1 ];
			stp2 = stp->s_scopes[ d1+1 ];
			if( stp1->s_inner ){
				stp3 = stp1->s_inner;
				find_gi_len( stp3->s_index,
					descr, &minlen3, &maxlen3 );
				stp1->s_minilen = minlen3;
				stp1->s_maxilen = maxlen3;
				gminlen += minlen3;
				if( gmaxlen != UNBOUNDED ){
					if( maxlen3 == UNBOUNDED )
						gmaxlen = UNBOUNDED;
					else
						gmaxlen += maxlen3;
				}
			}else{
				stp1->s_minilen = 0;
				stp1->s_maxilen = 0;
			}
			gminlen += stp2->s_minlen;
			if( gmaxlen != UNBOUNDED ){
				if( stp2->s_maxlen == UNBOUNDED )
					gmaxlen = UNBOUNDED;
				else
					gmaxlen += stp2->s_maxlen;
			}
		} 
		stp->s_minglen = gminlen;
		stp->s_maxglen = gmaxlen;
		*tminlen += gminlen;
		if( *tmaxlen != UNBOUNDED ){
			if( gmaxlen == UNBOUNDED )
				*tmaxlen = UNBOUNDED;
			else
				*tmaxlen += gmaxlen; 
		}
		if( stp->s_next )
			nd = stp->s_next->s_index;
		else
			break;
	} 
}

static	void	find_limits( int fd, STREL_T descr[] )
{
	int	d, nd, s;
	STREL_T	*stp, *stp1, *stp2;

	for( d = fd; ; d = nd ){
		stp = &descr[ d ];
		find_1_limit( stp, descr );
		for( s = 1; s < stp->s_n_scopes; s++ ){
			stp1 = stp->s_scopes[ s - 1 ];
			stp2 = stp->s_scopes[ s ];
			if( stp1->s_index + 1 < stp2->s_index )
				find_limits( stp1->s_index+1, descr );
			find_1_limit( stp2, descr );
		}
		stp1 = stp->s_next;
		if( stp1 == NULL )
			return;
		else
			nd = stp1->s_index;
	} 
}

static	void	find_1_limit( STREL_T *stp, STREL_T descr[] )
{

	find_start( stp, descr );
	find_stop( stp, descr );
}

static	void	find_start( STREL_T *stp, STREL_T descr[] )
{

	if( stp->s_scope == UNDEF ){	/* ss	*/
		stp->s_start.a_l2r = TRUE;
		stp->s_start.a_offset = 0;
	}else if( stp->s_scope == 0 ){	/* start a group	*/
		stp->s_start.a_l2r = TRUE;
		stp->s_start.a_offset = 0;
	}else{
		if( RM_R2L( stp->s_type ) ){
			if( closes_unbnd( stp, descr ) ){
				stp->s_start.a_offset =
					min_suffixlen( stp, descr );
				stp->s_start.a_l2r = FALSE;
			}else{
				stp->s_start.a_offset =
					max_prefixlen( stp, descr );
				stp->s_start.a_offset += stp->s_maxlen - 1;
				stp->s_start.a_l2r = TRUE;
			}
		}else{
			stp->s_start.a_offset = min_prefixlen( stp, descr );
			stp->s_start.a_l2r = TRUE;
		}
	}
}

static	void	find_stop( STREL_T *stp, STREL_T descr[] )
{

	if( RM_R2L( stp->s_type ) ){
		stp->s_stop.a_offset =
			stp->s_minlen + min_prefixlen( stp, descr ) - 1;
		stp->s_stop.a_l2r = TRUE;
	}else{
		stp->s_stop.a_offset = 
			stp->s_minlen + min_suffixlen( stp, descr );
		if( stp->s_stop.a_offset > 0 )
			stp->s_stop.a_offset--;
		stp->s_stop.a_l2r = FALSE;
	}
}

static	int	closes_unbnd( STREL_T *stp, STREL_T descr[] )
{

	if( stp->s_maxlen == UNBOUNDED )
		return( 1 );
	if( max_prefixlen( stp, descr ) == UNBOUNDED )
		return( 1 );
	return( 0 );
}

static	int	min_prefixlen( STREL_T *stp, STREL_T descr[] )
{
	STREL_T	*stp0, *stp1;
	int	s, plen;

	if( stp->s_scope == UNDEF )
		return( 0 );
	stp0 = stp->s_scopes[ 0 ];
	for( plen = 0, s = stp->s_index - 1; s >= stp0->s_index; s-- ){
		stp1 = &descr[ s ];
		plen += stp1->s_minlen;
	}
	return( plen );
}

static	int	max_prefixlen( STREL_T *stp, STREL_T descr[] )
{
	STREL_T	*stp0, *stp1;
	int	s, plen;

	if( stp->s_scope == UNDEF )
		return( 0 );
	stp0 = stp->s_scopes[ 0 ];
	for( plen = 0, s = stp->s_index - 1; s >= stp0->s_index; s-- ){
		stp1 = &descr[ s ];
		if( stp1->s_maxlen == UNBOUNDED )
			return( UNBOUNDED );
		else
			plen += stp1->s_maxlen;
	}
	return( plen );
}

static	int	min_suffixlen( STREL_T *stp, STREL_T descr[] )
{
	STREL_T	*stp0, *stp1, *stpn;
	int	s, slen;

	slen = 0;
	if( stp->s_scope == UNDEF ){
		for( stp1 = stp->s_next; stp1; stp1 = stp1->s_next )
			if( stp1->s_scope == 0 )
				slen += stp1->s_minglen;
			else
				slen += stp1->s_minlen;
		return( slen );
	}

	slen = 0;
	stpn = stp->s_scopes[ stp->s_n_scopes - 1 ];
	for( s = stp->s_index + 1; s <= stpn->s_index; s++ ){
		stp1 = &descr[ s ];
		slen += stp1->s_minlen;
	}
	stp0 = stp->s_scopes[ 0 ];
	for( stp1 = stp0->s_next; stp1; stp1 = stp1->s_next )
		slen += stp1->s_minlen;
	return( slen );
}

static	void	find_search_order( int fd, STREL_T descr[] )
{
	int	d, nd, s;
	STREL_T	*stp, *stp1, *stp2;
	SEARCH_T	*srp;

	for( d = fd; ; d = nd ){
		stp = &descr[ d ];
		switch( stp->s_type ){
		case SYM_SS :
			srp = ( SEARCH_T * )malloc( sizeof( SEARCH_T ) );
			if( srp == NULL ){
				rm_error = TRUE;
				LOG_ERROR("%s:%d can't allocate srp for ss.", rm_wdfname, stp->s_lineno);
				exit(1);
			}
			srp->s_descr = stp;
			stp->s_searchno = rm_n_searches;
			srp->s_forward = NULL;
			srp->s_backup = NULL;
			srp->s_zero = UNDEF;
			srp->s_dollar = UNDEF;
			rm_searches[ rm_n_searches ] = srp;
			rm_n_searches++;
			break;

		case SYM_H5 :
			if( stp->s_attr[ SA_PROPER ] ){
				srp = ( SEARCH_T * )malloc(sizeof( SEARCH_T ));
				if( srp == NULL ){
					rm_error = TRUE;
					LOG_ERROR("%s:%d can't allocate srp for hlx h5.", rm_wdfname, stp->s_lineno);
					exit(1);
				}
				srp->s_descr = stp;
				stp->s_searchno = rm_n_searches;
				srp->s_forward = NULL;
				srp->s_backup = NULL;
				rm_searches[ rm_n_searches ] = srp;
				rm_n_searches++;
				stp1 = stp->s_inner;
				if( stp1 != NULL )
					find_search_order( stp1->s_index,
						descr );
			}else{	/* snarl */
				for( s = 0; s < stp->s_n_scopes; s++ ){
					stp1 = stp->s_scopes[ s ];
					if( stp1->s_type == SYM_H5 ){
						srp = ( SEARCH_T * )
						    malloc(sizeof(SEARCH_T));
						if( srp == NULL ){
							rm_error = TRUE;
							LOG_ERROR("%s:%d can't allocate srp for hlx h5.", rm_wdfname, stp->s_lineno);
							exit(1);
						}
						srp->s_descr = stp1;
						stp1->s_searchno=rm_n_searches;
						srp->s_forward = NULL;
						srp->s_backup = NULL;
						rm_searches[rm_n_searches]=srp;
						rm_n_searches++;
					}
				}
				for( s = 0; s < stp->s_n_scopes; s++ ){
					stp1 = stp->s_scopes[ s ];
					stp2 = stp1->s_inner;
					if( stp2 != NULL )
						find_search_order(
							stp2->s_index, descr );
				}
			}
			break;

		case SYM_P5 :
			srp = ( SEARCH_T * )malloc( sizeof( SEARCH_T ) );
			if( srp == NULL ){
				rm_error = TRUE;
				LOG_ERROR("%s:%d can't allocate srp for p-hlx p5.", rm_wdfname, stp->s_lineno);
				exit(1);
			}
			srp->s_descr = stp;
			stp->s_searchno = rm_n_searches;
			srp->s_forward = NULL;
			srp->s_backup = NULL;
			rm_searches[ rm_n_searches ] = srp;
			rm_n_searches++;
			stp1 = stp->s_inner;
			if( stp1 != NULL )
				find_search_order( stp1->s_index, descr );
			break;

		case SYM_T1 :
			srp = ( SEARCH_T * )malloc( sizeof( SEARCH_T ) );
			if( srp == NULL ){
				rm_error = TRUE;
				LOG_ERROR("%s:%d can't allocate srp for triple t1.", rm_wdfname, stp->s_lineno);
				exit(1);
			}
			srp->s_descr = stp;
			stp->s_searchno = rm_n_searches;
			srp->s_forward = NULL;
			srp->s_backup = NULL;
			rm_searches[ rm_n_searches ] = srp;
			rm_n_searches++;
			stp1 = stp->s_inner;
			if( stp1 != NULL )
				find_search_order( stp1->s_index, descr );
			stp1 = stp->s_scopes[ 1 ];
			stp2 = stp1->s_inner;
			if( stp2 != NULL )
				find_search_order( stp2->s_index, descr );
			break;

		case SYM_Q1 :
			srp = ( SEARCH_T * )malloc( sizeof( SEARCH_T ) );
			if( srp == NULL ){
				rm_error = TRUE;
				LOG_ERROR("%s:%d can't allocate srp for quad q1.", rm_wdfname, stp->s_lineno);
				exit(1);
			}
			srp->s_descr = stp;
			stp->s_searchno = rm_n_searches;
			srp->s_forward = NULL;
			srp->s_backup = NULL;
			rm_searches[ rm_n_searches ] = srp;
			rm_n_searches++;
			stp1 = stp->s_inner;
			if( stp1 != NULL )
				find_search_order( stp1->s_index, descr );
			stp1 = stp->s_scopes[ 1 ];
			stp2 = stp1->s_inner;
			if( stp2 != NULL )
				find_search_order( stp2->s_index, descr );
			stp1 = stp->s_scopes[ 2 ];
			stp2 = stp1->s_inner;
			if( stp2 != NULL )
				find_search_order( stp2->s_index, descr );
			break;

		case SYM_H3 :
		case SYM_P3 :
		case SYM_T2 :
		case SYM_T3 :
		case SYM_Q2 :
		case SYM_Q3 :
		case SYM_Q4 :
			break;

		default :
			rm_error = TRUE;
			LOG_ERROR("%s:%d illegal symbol %d.", rm_wdfname, stp->s_lineno, stp->s_type);
			exit(1);
			break;
		}
		stp1 = stp->s_next;
		if( stp1 == NULL )
			return;
		else
			nd = stp1->s_index;
	}
}

static	void	set_search_order_links( int n_searches, SEARCH_T *searches[] )
{
	int	s;
	STREL_T	*stp, *stp1;

	for( s = 0; s < n_searches - 1; s++ )
		searches[ s ]->s_forward = searches[ s + 1 ]->s_descr;
	for( s = 1; s < n_searches; s++ ){
		stp = searches[ s ]->s_descr;
		if( stp->s_prev != NULL )
			searches[ s ]->s_backup = stp->s_prev;
		else{ 
			stp1 = stp->s_outer;
			if( stp1 == NULL )
				searches[ s ]->s_backup = NULL;
			else if( stp1->s_attr[ SA_PROPER ] )
				searches[ s ]->s_backup = stp1;
			else{	/* pknot	*/
				stp1 = stp1->s_scopes[ 0 ];
				searches[ s ]->s_backup = stp1;
			}
		}
	}
}

static	void	optimize_query( void )
{
	int	d;
	float	ecnt, o_ecnt;
	int	bpos, blen, bmin, lmin, lmax, rmin, rmax;
	STREL_T	*stp;

	/* look for best literal	*/
	for( o_ecnt = 0, rm_o_stp = NULL, d = 0; d < rm_n_descr; d++ ){
		stp = &rm_descr[ d ];
		if( stp->s_seq != NULL ){
			ecnt = stp->s_bestpat.b_ecnt - stp->s_mismatch;
			if( ecnt < rm_args->a_o_emin )
				continue;
			if( ecnt > o_ecnt ){
				o_ecnt = ecnt;
				rm_o_stp = stp;
			}
		}
	}
	if( rm_o_stp != NULL ){

		bpos = rm_o_stp->s_bestpat.b_pos;
		blen = rm_o_stp->s_bestpat.b_len;
		bmin = rm_o_stp->s_bestpat.b_minlen;
		lmin = rm_o_stp->s_bestpat.b_lminlen;
		lmax = rm_o_stp->s_bestpat.b_lmaxlen;
		rmin = rm_o_stp->s_bestpat.b_rminlen;
		rmax = rm_o_stp->s_bestpat.b_rmaxlen;

		rm_o_expbuf = ( char * )
			mm_regdup( &rm_o_stp->s_expbuf[ bpos ], blen );
		if( rm_o_expbuf == NULL ){
			rm_error = TRUE;
			LOG_ERROR("%s:%d can't allocate rm_o_expbuf.", rm_wdfname, rm_o_stp->s_lineno);
			exit(1);
		}

		if( rm_o_stp->s_maxlen != UNBOUNDED ){
			if( lmax == UNBOUNDED ){
				lmax = rm_o_stp->s_maxlen -
					bmin - rmin; 
				rm_o_stp->s_bestpat.b_lmaxlen = lmax;
			}
			if( rmax == UNBOUNDED ){
				rmax = rm_o_stp->s_maxlen -
					bmin - lmin; 
				rm_o_stp->s_bestpat.b_rmaxlen = rmax;
			}
		}

		for( d = 0; d < rm_o_stp->s_index; d++ ){
			stp = &rm_descr[ d ];
			lmin += stp->s_minlen;
			if( lmax != UNBOUNDED ){
				if( stp->s_maxlen == UNBOUNDED )
					lmax = UNBOUNDED;
				lmax += stp->s_maxlen;
			}
		}
		for( d = rm_o_stp->s_index + 1; d < rm_n_descr; d++ ){
			stp = &rm_descr[ d ];
			rmin += stp->s_minlen;
			if( rmax != UNBOUNDED ){
				if( stp->s_maxlen == UNBOUNDED )
					rmax = UNBOUNDED;
				rmax += stp->s_maxlen;
			}
		}
		rm_o_stp->s_bestpat.b_lminlen = lmin;
		rm_o_stp->s_bestpat.b_lmaxlen = lmax;
		rm_o_stp->s_bestpat.b_rminlen = rmin;
		rm_o_stp->s_bestpat.b_rmaxlen = rmax;

		if( rm_o_stp->s_bestpat.b_lmaxlen == UNBOUNDED )
			rm_o_stp = NULL;
	}
}

#if 0
static	void	dump_bestpat( FILE *fp, STREL_T *stp )
{
	float	o_ecnt;
	int	o_pos, o_len;
	int	o_bmin, o_bmax, o_lmin, o_lmax, o_rmin, o_rmax;

	o_ecnt = stp->s_bestpat.b_ecnt;
	o_pos = stp->s_bestpat.b_pos;
	o_len = stp->s_bestpat.b_len;
	o_bmin = stp->s_bestpat.b_minlen;
	o_bmax = stp->s_bestpat.b_maxlen;
	o_lmin = stp->s_bestpat.b_lminlen;
	o_lmax = stp->s_bestpat.b_lmaxlen;
	o_rmin = stp->s_bestpat.b_rminlen;
	o_rmax = stp->s_bestpat.b_rmaxlen;

	fprintf( fp, "bestpat: descr[%d], d_minl = %d, d_maxl = ",
		stp->s_index, rm_o_stp->s_minlen );
	if( stp->s_maxlen == UNBOUNDED )
		fprintf( fp, "UNBND" );
	else
		fprintf( fp, "%d", rm_o_stp->s_maxlen );
	fprintf( fp, "\n" );

	fprintf( fp,
	"bestpat: subpat: ecnt = %4.2f, pos,len = %d,%d\n",
		o_ecnt, o_pos, o_len );
	fprintf( fp, "bestpat: subpat: p_minl = %d, p_maxl = %d\n",
		o_bmin, o_bmax );
	fprintf( fp, "bestpat: subpat: lctx = %d:", o_lmin );
	if( o_lmax == UNBOUNDED )
		fprintf( fp, "UNBND" );
	else
		fprintf( fp, "%d", o_lmax );
	fprintf( fp, ", rctx = %d:", o_rmin );
	if( o_rmax == UNBOUNDED )
		fprintf( fp, "UNBND" );
	else
		fprintf( fp, "%d", o_rmax );
	fprintf( fp, "\n" );
}
#endif
